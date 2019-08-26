#include "ppcommon.h"

struct NkppState *nkppStateCreate(
    struct NkppErrorState *errorState,
    struct NkppMemoryCallbacks *memoryCallbacks)
{
    NkppMallocWrapper localMallocWrapper;
    NkppFreeWrapper localFreeWrapper;
    void *userData;
    struct NkppState *ret;

    nkppSanityCheck();

    localMallocWrapper =
        (memoryCallbacks && memoryCallbacks->mallocWrapper) ?
        memoryCallbacks->mallocWrapper : nkppDefaultMallocWrapper;
    localFreeWrapper =
        (memoryCallbacks && memoryCallbacks->freeWrapper) ?
        memoryCallbacks->freeWrapper : nkppDefaultFreeWrapper;
    userData = memoryCallbacks ? memoryCallbacks->userData : NULL;

    ret = localMallocWrapper(NULL, userData, sizeof(struct NkppState));

    if(ret) {
        ret->str = NULL;
        ret->index = 0;
        ret->lineNumber = 1;
        ret->outputLineNumber = 1;
        ret->output = NULL;
        ret->outputCapacity = 0;
        ret->outputLength = 0;
        ret->macros = NULL;
        ret->writePositionMarkers = nkfalse;
        ret->updateMarkers = nkfalse;
        ret->errorState = errorState;
        ret->memoryCallbacks = memoryCallbacks;
        ret->recursionLevel = 0;
        ret->concatenationEnabled = nkfalse;
        ret->preprocessingIfExpression = nkfalse;
        ret->tokensOnThisLine = 0;
        ret->conditionalStack = NULL;

        // Memory callbacks now set up. Can use normal functions that
        // require allocations.
        ret->filename = nkppStrdup(ret, "<unknown>");
        if(!ret->filename) {
            localFreeWrapper(NULL, userData, ret);
            return NULL;
        }
    }

    return ret;
}

void nkppStateDestroy(struct NkppState *state)
{
    NkppFreeWrapper localFreeWrapper =
        (state->memoryCallbacks && state->memoryCallbacks->freeWrapper) ?
        state->memoryCallbacks->freeWrapper : nkppDefaultFreeWrapper;
    void *userData =
        state->memoryCallbacks ? state->memoryCallbacks->userData : NULL;

    struct NkppMacro *currentMacro = NULL;
    struct NkppStateConditional *currentConditional = NULL;

    currentMacro = state->macros;
    while(currentMacro) {
        struct NkppMacro *next = currentMacro->next;
        nkppMacroDestroy(state, currentMacro);
        currentMacro = next;
    }

    currentConditional = state->conditionalStack;
    while(currentConditional) {
        struct NkppStateConditional *next = currentConditional->next;
        nkppFree(state, currentConditional);
        currentConditional = next;
    }

    nkppFree(state, state->output);
    nkppFree(state, state->filename);

    // Final NkppState allocation was not allocated through
    // nkppMalloc. Use the memory callback directly.
    localFreeWrapper(NULL, userData, state);
}

// This should only ever be called when we're about to output a
// newline anyway.
nkbool nkppStateWritePositionMarker(struct NkppState *state)
{
    char numberStr[128];
    char *escapedFilenameStr =
        nkppEscapeString(state, state->filename);
    nkbool ret = nktrue;

    if(!escapedFilenameStr) {
        return nkfalse;
    }

    ret &= nkppStateOutputAppendString(state, "#file \"");
    ret &= nkppStateOutputAppendString(state, escapedFilenameStr);
    ret &= nkppStateOutputAppendString(state, "\"");

    sprintf(
        numberStr, "\n#line %ld\n",
        (long)state->lineNumber);
    ret &= nkppStateOutputAppendString(state, numberStr);

    state->outputLineNumber = state->lineNumber;

    nkppFree(state, escapedFilenameStr);

    return ret;
}

nkbool nkppStateOutputAppendString(struct NkppState *state, const char *str)
{
    if(!str) {

        return nktrue;

    } else {

        nkuint32_t len = nkppStrlen(str);
        nkuint32_t i;

        for(i = 0; i < len; i++) {
            if(!nkppStateOutputAppendChar(state, str[i])) {
                return nkfalse;
            }
        }
    }

    return nktrue;
}

nkbool nkppStateOutputAppendChar_real(struct NkppState *state, char c)
{
    nkuint32_t oldLen;
    nkuint32_t newLen;
    nkuint32_t allocLen;
    nkbool overflow = nkfalse;
    char *newChunk;

    // Calculate new size with overflow check.
    oldLen = state->outputLength;
    NK_CHECK_OVERFLOW_UINT_ADD(oldLen, 1, newLen, overflow);
    NK_CHECK_OVERFLOW_UINT_ADD(newLen, 1, allocLen, overflow);
    if(overflow) {
        return nkfalse;
    }

    if(allocLen > state->outputCapacity) {

        // Need to reallocate.

        nkuint32_t newCapacity;

        NK_CHECK_OVERFLOW_UINT_MUL(
            state->outputCapacity, 2,
            newCapacity, overflow);
        if(overflow) {
            return nkfalse;
        }

        if(newCapacity < 8) {
            newCapacity = 8;
        }

        newChunk = nkppRealloc(state, state->output, newCapacity);

        if(!newChunk) {
            return nkfalse;
        }

        state->output = newChunk;
        state->outputCapacity = newCapacity;
    }

    // Add the new character and a null terminator.
    state->outputLength++;
    state->output[oldLen] = c;
    state->output[newLen] = 0;

    return nktrue;
}

nkbool nkppStateDebugOutputNumber(struct NkppState *state, nkuint32_t n)
{
    nkuint32_t lnMask = 10000;
    nkbool ret = nktrue;

    while(lnMask && !(n / lnMask)) {
        ret &= nkppStateOutputAppendChar_real(state, ' ');
        n %= lnMask;
        lnMask /= 10;
    }

    while(lnMask) {
        ret &= nkppStateOutputAppendChar_real(state, '0' + (n / lnMask));
        n %= lnMask;
        lnMask /= 10;
    }

    return ret;
}

nkbool nkppStateDebugOutputLineStart(struct NkppState *state)
{
    nkbool ret = nktrue;

    // Add truncated filename with padding.
    {
        const char *filename = state->filename ? state->filename : "???";
        nkuint32_t fnameLen = nkppStrlen(filename);
        nkuint32_t i;
        for(i = 0; i < 12; i++) {
            if(i < fnameLen) {
                ret = ret && nkppStateOutputAppendChar_real(state, filename[i]);
            } else {
                ret = ret && nkppStateOutputAppendChar_real(state, ' ');
            }
        }
    }

    // Separator between filename and line number.
    ret = ret && nkppStateOutputAppendChar_real(state, ':');

    // Add line numbers for both input and output.
    ret = ret && nkppStateDebugOutputNumber(state, state->lineNumber);
    ret = ret && nkppStateDebugOutputNumber(state, state->outputLineNumber);

    // Add number of nested "if" results.
    ret = ret && nkppStateDebugOutputNumber(state, 0);
    ret = ret && nkppStateDebugOutputNumber(state, 0);

    // Add a marker indicating that input/output line numbers may be
    // out of sync and it's an appropriate place to put a marker to
    // correct for it.
    ret = ret && nkppStateOutputAppendChar_real(state, ' ');
    ret = ret && nkppStateOutputAppendChar_real(state, state->updateMarkers ? 'U' : ' ');

    // Space.
    ret = ret && nkppStateOutputAppendChar_real(state, ' ');

    // Add a marker to indicate if the input and output are *actually*
    // out of sync.
    if(state->outputLineNumber != state->lineNumber) {
        ret = ret && nkppStateOutputAppendChar_real(state, '-');
    } else {
        ret = ret && nkppStateOutputAppendChar_real(state, '|');
    }

    // Final space.
    ret = ret && nkppStateOutputAppendChar_real(state, ' ');

    return ret;
}

nkbool nkppStateOutputAppendChar(struct NkppState *state, char c)
{
    nkbool ret = nktrue;

    if(state->writePositionMarkers) {

        // If this is the first character on a line...
        if(!state->output || state->output[state->outputLength - 1] == '\n') {

            // FIXME: Make this optional.
            ret = nkppStateDebugOutputLineStart(state) && ret;

            if(state->outputLineNumber != state->lineNumber) {

                // Add in a position marker and set the output line
                // number.
                if(state->updateMarkers) {
                    state->outputLineNumber = state->lineNumber;
                    state->updateMarkers = nkfalse;
                    ret = ret && nkppStateWritePositionMarker(state);

                    // Send in a character we purposely won't do anything
                    // with just to pump the line-start debug info stuff.
                    ret = ret && nkppStateOutputAppendChar(state, 0);
                }
            }

            // If we hit this case, we may have had a redundant use of
            // the updateMarkers flag. So just un-set it because we're
            // fine now.
            if(state->lineNumber == state->outputLineNumber) {
                state->updateMarkers = nkfalse;
            }
        }
    }

    if(c == '\n') {
        state->outputLineNumber++;
    }

    if(c) {
        if(nkppStateConditionalOutputPassed(state) || c == '\n') {
            ret = ret && nkppStateOutputAppendChar_real(state, c);
        }
    }

    return ret;
}

nkbool nkppStateInputSkipChar(struct NkppState *state, nkbool output)
{
    nkbool ret = nktrue;

    assert(state->str[state->index]);

    if(output) {
        if(!nkppStateOutputAppendChar(state, state->str[state->index])) {
            ret = nkfalse;
        }
    }

    if(state->str && state->str[state->index] == '\n') {
        state->lineNumber++;
        state->tokensOnThisLine = 0;

        // FIXME: Remove this.
        printf("Line: %ld\n", (long)state->lineNumber);
    }

    state->index++;

    return ret;
}

nkbool nkppStateInputSkipWhitespaceAndComments(
    struct NkppState *state,
    nkbool output,
    nkbool stopAtNewline)
{
    if(!state->str) {
        return nktrue;
    }

    while(state->str[state->index]) {

        // Skip comments (but output them).
        if(state->str[state->index] == '/' && state->str[state->index + 1] == '/') {

            // C++-style comment.
            while(state->str[state->index] && state->str[state->index] != '\n') {
                if(!nkppStateInputSkipChar(state, output)) {
                    return nkfalse;
                }
            }

        } else if(state->str[state->index] == '/' && state->str[state->index + 1] == '*') {

            // C-style comment.

            // Skip initial comment maker.
            if(!nkppStateInputSkipChar(state, output) || !nkppStateInputSkipChar(state, output)) {
                return nkfalse;
            }

            while(state->str[state->index] && state->str[state->index + 1]) {
                if(state->str[state->index] == '*' && state->str[state->index + 1] == '/') {
                    if(!nkppStateInputSkipChar(state, output) || !nkppStateInputSkipChar(state, output)) {
                        return nkfalse;
                    }
                    break;
                }
                if(!nkppStateInputSkipChar(state, output)) {
                    return nkfalse;
                }
            }

        } else if(!nkppIsWhitespace(state->str[state->index])) {

            // Non-whitespace, non-comment character found.
            break;

        }

        // Bail out when we get to the end of the line.
        if(state->str[state->index] == '\n') {
            if(stopAtNewline) {
                break;
            }
        }

        // Check for end of buffer.
        if(!state->str[state->index]) {
            break;
        }

        if(!nkppStateInputSkipChar(state, output)) {
            return nkfalse;
        }
    }

    return nktrue;
}

char *nkppStateInputReadBytes(
    struct NkppState *state,
    nkuint32_t count)
{
    nkuint32_t len = 0;
    nkbool overflow = nkfalse;
    char *ret = NULL;
    nkuint32_t i;

    NK_CHECK_OVERFLOW_UINT_ADD(count, 1, len, overflow);
    if(overflow) {
        return NULL;
    }

    ret = nkppMalloc(state, len);
    if(!ret) {
        return NULL;
    }

    for(i = 0; i < count; i++) {
        ret[i] = state->str[state->index];
        if(state->str[state->index] == 0) {
            break;
        }
        nkppStateInputSkipChar(state, nkfalse);
    }

    ret[i] = 0;

    return ret;
}

nkbool nkppStateInputGetNextToken_stringStartsWith(
    const char *needle,
    const char *haystack)
{
    while(*needle) {
        if(*needle != *haystack) {
            return nkfalse;
        }
        if(!*haystack) {
            return nkfalse;
        }
        if(!*needle) {
            break;
        }

        haystack++;
        needle++;
    }
    return nktrue;
}

// This is pretty ugly. In order to detect "defined()" expressions
// inside "#if" and "#elif" arguments, we actually need to go
// backwards through the output to see if the last thing we emitted
// was the start of that expression. We need to do this so we *don't*
// evaluate the macro inside the expression.
//
// The "defined" text can make it into our expression in many
// different ways, and the only consistent way to deal with it is
// going to be to check the output.
nkbool nkppStateInputGetNextToken_checkForDefinedExpression(
    struct NkppState *state)
{
    nkuint32_t i = state->index - 1;
    nkuint32_t tokenEnd;

    // Skip the identifier we parsed that lead us here.
    while(i != NK_UINT_MAX) {
        if(nkppIsValidIdentifierCharacter(state->str[i], nktrue)) {
            i--;
        } else {
            break;
        }
    }
    if(i == NK_UINT_MAX) {
        return nkfalse;
    }

    // Skip whitespace.
    while(i != NK_UINT_MAX) {
        if(nkppIsWhitespace(state->str[i])) {
            i--;
        } else {
            break;
        }
    }
    if(i == NK_UINT_MAX) {
        return nkfalse;
    }

    // Find '('.
    if(state->str[i] != '(') {
        return nkfalse;
    }

    // Go back from the '('.
    i--;
    if(i == NK_UINT_MAX) {
        return nkfalse;
    }

    // Skip more whitespace.
    while(i != NK_UINT_MAX) {
        if(nkppIsWhitespace(state->str[i])) {
            i--;
        } else {
            break;
        }
    }
    if(i == NK_UINT_MAX) {
        return nkfalse;
    }

    // Find "defined".
    tokenEnd = i + 1;
    while(i != NK_UINT_MAX) {
        if(nkppIsValidIdentifierCharacter(state->str[i], nktrue)) {
            i--;
        } else {
            i++;
            break;
        }
    }
    if(i == NK_UINT_MAX) {
        return nkfalse;
    }

    // Check to see if the identifier we just blew past was, indeed,
    // the "defined" text we're looking for.
    if(nkppStateInputGetNextToken_stringStartsWith("defined", state->str + i) &&
        tokenEnd - i == nkppStrlen("defined"))
    {
        return nktrue;
    }

    return nkfalse;
}

struct NkppToken *nkppStateInputGetNextToken(
    struct NkppState *state,
    nkbool outputWhitespace)
{
    nkbool success = nktrue;
    struct NkppToken *ret = nkppMalloc(
        state, sizeof(struct NkppToken));
    if(!ret) {
        return NULL;
    }

    ret->str = NULL;
    ret->type = NK_PPTOKEN_UNKNOWN;

    if(!nkppStateInputSkipWhitespaceAndComments(state, outputWhitespace, nkfalse)) {
        nkppFree(state, ret);
        return NULL;
    }

    ret->lineNumber = state->lineNumber;

    // Bail out if we're at the end.
    if(!state->str[state->index]) {
        nkppFree(state, ret);
        return NULL;
    }

    // FIXME: This list is getting long enough that I think we need a
    // table of values that we just search through.

    if(nkppIsValidIdentifierCharacter(state->str[state->index], nktrue)) {

        // Read identifiers (and directives).
        ret->str = nkppStateInputReadIdentifier(state);
        if(!ret->str) {
            nkppFree(state, ret);
            return NULL;
        }

        if(!nkppStrcmp(ret->str, "defined")) {
            ret->type = NK_PPTOKEN_DEFINED;
        } else {
            ret->type = NK_PPTOKEN_IDENTIFIER;
        }

    } else if(nkppIsDigit(state->str[state->index])) {

        // Read number.
        ret->str = nkppStateInputReadInteger(state);
        ret->type = NK_PPTOKEN_NUMBER;

    } else if(state->str[state->index] == '"') {

        // Read quoted string.
        ret->str = nkppStateInputReadQuotedString(state);
        ret->type = NK_PPTOKEN_QUOTEDSTRING;

    } else if(state->str[state->index] == '#' &&
        state->str[state->index+1] == '#')
    {
        // Double-hash (concatenation) symbol.
        ret->str = nkppStateInputReadBytes(state, 2);
        ret->type = NK_PPTOKEN_DOUBLEHASH;

    } else if(state->str[state->index] == '#') {

        // Hash symbol.
        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_HASH;

    } else if(state->str[state->index] == ',') {

        // Comma.
        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_COMMA;

    } else if(state->str[state->index] == '(') {

        // Open parenthesis.
        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_OPENPAREN;

    } else if(state->str[state->index] == ')') {

        // Close parenthesis.
        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_CLOSEPAREN;

    } else if(state->str[state->index] == '<' && state->str[state->index + 1] == '<') {

        ret->str = nkppStateInputReadBytes(state, 2);
        ret->type = NK_PPTOKEN_LEFTSHIFT;

    } else if(state->str[state->index] == '>' && state->str[state->index + 1] == '>') {

        ret->str = nkppStateInputReadBytes(state, 2);
        ret->type = NK_PPTOKEN_RIGHTSHIFT;

    } else if(state->str[state->index] == '<' && state->str[state->index + 1] == '=') {

        ret->str = nkppStateInputReadBytes(state, 2);
        ret->type = NK_PPTOKEN_LESSTHANOREQUALS;

    } else if(state->str[state->index] == '>' && state->str[state->index + 1] == '=') {

        ret->str = nkppStateInputReadBytes(state, 2);
        ret->type = NK_PPTOKEN_GREATERTHANOREQUALS;

    } else if(state->str[state->index] == '>') {

        // Greater-than.
        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_GREATERTHAN;

    } else if(state->str[state->index] == '<') {

        // Less-than.
        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_LESSTHAN;

    } else if(state->str[state->index] == '!' && state->str[state->index + 1] == '=') {

        ret->str = nkppStateInputReadBytes(state, 2);
        ret->type = NK_PPTOKEN_NOTEQUAL;

    } else if(state->str[state->index] == '=' && state->str[state->index + 1] == '=') {

        // Equality comparison.
        ret->str = nkppStateInputReadBytes(state, 2);
        ret->type = NK_PPTOKEN_COMPARISONEQUALS;

    } else if(state->str[state->index] == '-') {

        // Minus.
        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_MINUS;

    } else if(state->str[state->index] == '~') {

        // Tilde.
        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_TILDE;

    } else if(state->str[state->index] == '!') {

        // Exclamation.
        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_EXCLAMATION;

    } else if(state->str[state->index] == '*') {

        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_ASTERISK;

    } else if(state->str[state->index] == '/') {

        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_SLASH;

    } else if(state->str[state->index] == '%') {

        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_PERCENT;

    } else if(state->str[state->index] == '+') {

        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_PLUS;

    } else if(state->str[state->index] == '^') {

        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_BINARYXOR;

    } else if(state->str[state->index] == '|' && state->str[state->index + 1] == '|') {

        ret->str = nkppStateInputReadBytes(state, 2);
        ret->type = NK_PPTOKEN_LOGICALOR;

    } else if(state->str[state->index] == '|') {

        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_BINARYOR;

    } else if(state->str[state->index] == '&' && state->str[state->index + 1] == '&') {

        ret->str = nkppStateInputReadBytes(state, 2);
        ret->type = NK_PPTOKEN_LOGICALAND;

    } else if(state->str[state->index] == '&') {

        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_BINARYAND;

    } else if(state->str[state->index] == '?') {

        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_QUESTIONMARK;

    } else if(state->str[state->index] == ':') {

        ret->str = nkppStateInputReadBytes(state, 1);
        ret->type = NK_PPTOKEN_COLON;

    } else {

        // Unknown.
        ret->str = nkppStateInputReadBytes(state, 1);
    }

    if(!ret->str || !success) {
        nkppFree(state, ret->str);
        nkppFree(state, ret);
        ret = NULL;
    }

    state->tokensOnThisLine++;

    return ret;
}

char *nkppStateInputReadRestOfLine(
    struct NkppState *state,
    nkuint32_t *actualLineCount)
{
    nkbool lastCharWasBackslash = nkfalse;
    nkuint32_t lineStart = state->index;
    nkuint32_t lineEnd = lineStart;
    nkuint32_t lineLen = 0;
    nkuint32_t lineBufLen = 0;
    char *ret = NULL;
    nkbool overflow = nkfalse;

    while(state->str[state->index]) {

        if(state->str[state->index] == '/' && state->str[state->index + 1] == '*') {

            // C-style comment.

            // Skip initial comment maker.
            if(!nkppStateInputSkipChar(state, nktrue) || !nkppStateInputSkipChar(state, nktrue)) {
                return nkfalse;
            }

            while(state->str[state->index] && state->str[state->index + 1]) {
                if(state->str[state->index] == '*' && state->str[state->index + 1] == '/') {
                    if(!nkppStateInputSkipChar(state, nktrue) || !nkppStateInputSkipChar(state, nktrue)) {
                        return nkfalse;
                    }
                    break;
                }
                if(!nkppStateInputSkipChar(state, nktrue)) {
                    return nkfalse;
                }
            }

            lastCharWasBackslash = nkfalse;

        } else if(state->str[state->index] == '\\') {

            lastCharWasBackslash = !lastCharWasBackslash;

        } else if(state->str[state->index] == '\n') {

            if(actualLineCount) {
                (*actualLineCount)++;
            }

            if(lastCharWasBackslash) {

                // This is an escaped newline, so we're going to keep
                // going.
                lastCharWasBackslash = nkfalse;

            } else {

                // Skip that newline and bail out. We're done. Only
                // output if it's a newline to keep lines in sync
                // between input and output.
                if(!nkppStateInputSkipChar(state, state->str[state->index] == '\n')) {
                    return NULL;
                }
                break;

            }

        } else {

            lastCharWasBackslash = nkfalse;
        }

        // Skip this character. Only output if it's a newline to keep
        // lines in sync between input and output.
        if(!nkppStateInputSkipChar(state, state->str[state->index] == '\n')) {
            return NULL;
        }
    }

    // Save the whole line.
    lineEnd = state->index;

    if(lineEnd < lineStart) {
        return NULL;
    }
    lineLen = lineEnd - lineStart;

    // Add room for a NULL terminator.
    NK_CHECK_OVERFLOW_UINT_ADD(lineLen, 1, lineBufLen, overflow);
    if(overflow) {
        return NULL;
    }

    // Create and fill the return buffer.
    ret = nkppMalloc(state, lineBufLen);
    if(ret) {
        nkppMemcpy(ret, state->str + lineStart, lineLen);
        ret[lineLen] = 0;
    }

    return ret;
}

void nkppStateAddMacro(
    struct NkppState *state,
    struct NkppMacro *macro)
{
    macro->next = state->macros;
    state->macros = macro;
}

struct NkppMacro *nkppStateFindMacro(
    struct NkppState *state,
    const char *identifier)
{
    struct NkppMacro *currentMacro = state->macros;

    if(!identifier) {
        return NULL;
    }

    // Find the macro.
    while(currentMacro) {
        if(!nkppStrcmp(currentMacro->identifier, identifier)) {
            break;
        }
        currentMacro = currentMacro->next;
    }

    // Dynamically add in or modify __FILE__ and __LINE__ macros.
    if(!nkppStrcmp(identifier, "__FILE__") ||
        !nkppStrcmp(identifier, "__LINE__"))
    {
        // Lazy-create the macro if it doesn't exist.
        if(!currentMacro) {

            currentMacro =
                nkppMacroCreate(state);

            if(currentMacro) {
                if(nkppMacroSetIdentifier(
                        state, currentMacro, identifier))
                {
                    nkppStateAddMacro(
                        state, currentMacro);
                } else {
                    nkppMacroDestroy(
                        state, currentMacro);
                    currentMacro = NULL;
                }
            }
        }

        // Update to whatever it is now.
        if(currentMacro) {

            if(!nkppStrcmp(identifier, "__FILE__")) {

                char *escapedString = nkppEscapeString(state, state->filename);

                if(escapedString) {

                    nkuint32_t escapedStringLen = nkppStrlen(escapedString);
                    nkuint32_t quotedStringBufLen;
                    nkbool overflow = nkfalse;

                    NK_CHECK_OVERFLOW_UINT_ADD(
                        escapedStringLen, 3,
                        quotedStringBufLen, overflow);

                    if(!overflow) {
                        char *quotedString = nkppMalloc(state, quotedStringBufLen);

                        if(quotedString) {

                            // Add the actual quotes in.
                            quotedString[0] = '"';
                            nkppMemcpy(quotedString + 1, escapedString, escapedStringLen);
                            quotedString[escapedStringLen + 1] = '"';
                            quotedString[escapedStringLen + 2] = 0;

                            nkppMacroSetDefinition(state, currentMacro, quotedString);

                            nkppFree(state, quotedString);
                        }
                    }

                    nkppFree(state, escapedString);
                }

            } else if(!nkppStrcmp(identifier, "__LINE__")) {
                char tmp[128];
                sprintf(tmp, "%lu", (unsigned long)state->lineNumber);
                nkppMacroSetDefinition(state, currentMacro, tmp);
            }
        }
    }

    return currentMacro;
}

nkbool nkppStateDeleteMacro(
    struct NkppState *state,
    const char *identifier)
{
    struct NkppMacro *currentMacro = state->macros;
    struct NkppMacro **lastPtr = &state->macros;

    while(currentMacro) {

        if(!nkppStrcmp(currentMacro->identifier, identifier)) {

            *lastPtr = currentMacro->next;
            currentMacro->next = NULL;

            nkppMacroDestroy(
                state, currentMacro);

            return nktrue;
        }

        lastPtr = &currentMacro->next;
        currentMacro = currentMacro->next;

    }

    return nkfalse;
}

struct NkppState *nkppStateClone(
    struct NkppState *state,
    nkbool copyOutput)
{
    struct NkppState *ret = nkppStateCreate(
        state->errorState, state->memoryCallbacks);
    struct NkppMacro *currentMacro;
    struct NkppMacro **macroWritePtr;

    if(!ret) {
        return NULL;
    }

    ret->str = state->str; // Non-owning copy! (The source doesn't own it either.)
    ret->index = state->index;
    ret->lineNumber = state->lineNumber;
    ret->outputLineNumber = state->outputLineNumber;
    ret->recursionLevel = state->recursionLevel + 1;
    ret->preprocessingIfExpression = state->preprocessingIfExpression;

    // Copy output.
    if(copyOutput) {

        ret->output = nkppMalloc(state, state->outputCapacity);
        ret->outputCapacity = state->outputCapacity;
        ret->outputLength = state->outputLength;

        if(ret->outputCapacity) {
            if(!ret->output) {
                nkppStateDestroy(ret);
                return NULL;
            }
            nkppMemcpy(
                ret->output,
                state->output,
                state->outputLength + 1);
        }

        if(state->output && !ret->output) {
            nkppStateDestroy(ret);
            return NULL;
        }
    }

    // Note: We purposely don't write position markers or update
    // markers here.
    ret->errorState = state->errorState;
    if(!nkppStateSetFilename(ret, state->filename)) {
        nkppStateDestroy(ret);
        return NULL;
    }

    // Note: Nested if state is not copied.

    // Note: Macros that come in as arguments to another macro are not
    // copied.

    currentMacro = state->macros;
    macroWritePtr = &ret->macros;

    while(currentMacro) {

        if(!currentMacro->isArgumentName) {

            struct NkppMacro *clonedMacro =
                nkppMacroClone(state, currentMacro);
            if(!clonedMacro) {
                nkppStateDestroy(ret);
                return NULL;
            }

            *macroWritePtr = clonedMacro;
            macroWritePtr = &clonedMacro->next;
        }

        currentMacro = currentMacro->next;
    }

    return ret;
}

// ----------------------------------------------------------------------
// Read functions

char *nkppStateInputReadIdentifier(struct NkppState *state)
{
    const char *str = state->str;
    nkuint32_t *i = &state->index;

    nkuint32_t start = *i;
    nkuint32_t end;
    nkuint32_t len;
    char *ret;

    while(nkppIsValidIdentifierCharacter(str[*i], *i == start)) {
        (*i)++;
    }

    end = *i;

    len = end - start;

    // Check overflow and allocate.
    {
        nkbool overflow = nkfalse;
        nkuint32_t memLen = len;
        NK_CHECK_OVERFLOW_UINT_ADD(len, 1, memLen, overflow);
        if(overflow) {
            return NULL;
        }

        ret = nkppMalloc(state, memLen);
    }

    if(!ret) {
        return NULL;
    }

    nkppMemcpy(ret, str + start, len);
    ret[len] = 0;

    return ret;
}

char *nkppStateInputReadQuotedString(struct NkppState *state)
{
    const char *str = state->str;
    nkuint32_t *i = &state->index;
    nkuint32_t start = *i;
    nkuint32_t end;
    nkuint32_t len;
    char *ret;
    nkbool backslashed = nkfalse;

    // Should only be called on the starting quote.
    assert(str[*i] == '"');

    // Skip initial quote.
    if(!nkppStateInputSkipChar(state, nkfalse)) {
        return NULL;
    }

    while(str[*i]) {

        if(str[*i] == '\\') {

            backslashed = !backslashed;

        } else if(!backslashed && str[*i] == '"') {

            // Skip final quote.
            if(!nkppStateInputSkipChar(state, nkfalse)) {
                return NULL;
            }

            break;

        } else if(!backslashed && str[*i] == '\n') {

            // Reached an un-escaped newline. Bail out.
            break;

        } else {

            backslashed = nkfalse;

        }

        if(!nkppStateInputSkipChar(state, nkfalse)) {
            return NULL;
        }
    }

    end = *i;

    len = end - start;

    // Check overflow and allocate.
    {
        nkbool overflow = nkfalse;
        nkuint32_t memLen = len;
        NK_CHECK_OVERFLOW_UINT_ADD(len, 1, memLen, overflow);
        if(overflow) {
            return NULL;
        }

        ret = nkppMalloc(state, memLen);
    }

    if(!ret) {
        return NULL;
    }

    nkppMemcpy(ret, str + start, len);
    ret[len] = 0;

    return ret;
}

char *nkppStateInputReadInteger(struct NkppState *state)
{
    const char *str = state->str;
    nkuint32_t start = state->index;
    nkuint32_t end;
    nkuint32_t len;
    nkuint32_t bufLen;
    char *ret;
    nkbool overflow = nkfalse;
    nkbool hex = nkfalse;

    // Special case to skip past the base specifiers.
    if(str[state->index] == '0') {

        if(!nkppStateInputSkipChar(state, nkfalse)) {
            return NULL;
        }

        if(str[state->index] == 'x' || str[state->index] == 'b') {

            hex = str[state->index] == 'x';

            if(!nkppStateInputSkipChar(state, nkfalse)) {
                return NULL;
            }
        }
    }

    // Skip past other digits.
    while(
        nkppIsDigit(str[state->index]) ||
        (hex && nkppIsDigitHex(str[state->index])))
    {
        if(!nkppStateInputSkipChar(state, nkfalse)) {
            return NULL;
        }
    }

    end = state->index;
    len = end - start;

    // Add null terminator to length.
    NK_CHECK_OVERFLOW_UINT_ADD(len, 1, bufLen, overflow);
    if(overflow) {
        return NULL;
    }

    // Allocate.
    ret = nkppMalloc(state, bufLen);
    if(!ret) {
        return NULL;
    }

    // Copy and null-terminate.
    nkppMemcpy(ret, str + start, len);
    ret[len] = 0;

    return ret;
}

char *nkppStateInputReadMacroArgument(struct NkppState *state)
{
    // Create a pristine state to read the arguments with, because we
    // don't want to evaluate any macros at this point. That'll happen
    // on invocation.
    struct NkppState *readerState = NULL;
    nkuint32_t parenLevel = 0;
    char *ret = NULL;
    struct NkppToken *token = NULL;

    readerState = nkppStateCreate(
        state->errorState, state->memoryCallbacks);
    if(!readerState) {
        return NULL;
    }

    // Copy input and position.
    readerState->index = state->index;
    readerState->str = state->str;
    readerState->recursionLevel = state->recursionLevel + 1;

    // Start off with an allocated-but-empty string, because we have
    // to return something not-NULL to indicate a success.
    readerState->output = nkppMalloc(state, 1);
    if(!readerState->output) {
        nkppStateDestroy(readerState);
        return NULL;
    }
    readerState->output[0] = 0;

    // Skip whitespace up to the first token, but don't append
    // whitespace on this side of it.
    if(!nkppStateInputSkipWhitespaceAndComments(readerState, nkfalse, nkfalse)) {
        nkppStateDestroy(readerState);
        return NULL;
    }

    do {
        // Skip whitespace.
        if(!nkppStateInputSkipWhitespaceAndComments(readerState, nktrue, nkfalse)) {
            nkppStateDestroy(readerState);
            return NULL;
        }

        // Check to see if we're "done". (Zero-length arguments are
        // okay, so we have to do this at the start.)
        if(!parenLevel) {
            if(readerState->str[readerState->index] == ',' ||
                readerState->str[readerState->index] == ')')
            {
                break;
            }
        }

        // Read token and output it.
        token = nkppStateInputGetNextToken(readerState, nktrue);

        if(token) {

            // Check for '(', ')', and ','. Do stuff with paren level.
            if(token->type == NK_PPTOKEN_OPENPAREN) {
                parenLevel++;
            } else if(token->type == NK_PPTOKEN_CLOSEPAREN) {
                parenLevel--;
            }

            // Add it.
            if(!nkppStateOutputAppendString(readerState, token->str)) {
                nkppTokenDestroy(state, token);
                nkppStateDestroy(readerState);
                return NULL;
            }

            nkppTokenDestroy(state, token);
        }

    } while(token);

    // Update read position in the source state.
    state->index = readerState->index;
    state->lineNumber += readerState->lineNumber - 1;

    // Pull the output off the pp state and return it.
    ret = readerState->output;
    readerState->output = NULL;
    nkppStateDestroy(readerState);
    return ret;
}

// ----------------------------------------------------------------------

void nkppStateOutputClear(struct NkppState *state)
{
    nkppFree(state, state->output);
    state->output = NULL;
    state->outputCapacity = 0;
    state->outputLength = 0;
    state->outputLineNumber = 1;
}

void nkppStateAddError(
    struct NkppState *state,
    const char *errorMessage)
{
    if(state->errorState) {

        struct NkppError *newError =
            nkppMalloc(state, sizeof(struct NkppError));

        if(newError) {

            newError->filename = nkppStrdup(state, state->filename);
            newError->lineNumber = state->lineNumber;
            newError->text = nkppStrdup(state, errorMessage);
            newError->next = NULL;

            if(state->errorState->lastError) {
                state->errorState->lastError->next = newError;
            }

            if(!state->errorState->firstError) {
                state->errorState->firstError = newError;
            }

            state->errorState->lastError = newError;
        }

    } else {

        #if __WATCOMC__
        printf(
        #else
        fprintf(stderr,
        #endif
            "%s:%ld: %s\n",
            state->filename,
            (long)state->lineNumber, errorMessage);

    }
}

void nkppStateFlagFileLineMarkersForUpdate(
    struct NkppState *state)
{
    state->updateMarkers = nktrue;
}

nkbool nkppStateSetFilename(
    struct NkppState *state,
    const char *filename)
{
    if(state->filename) {
        nkppFree(state, state->filename);
        state->filename = NULL;
    }

    state->filename = nkppStrdup(state, filename);

    if(!state->filename) {
        return nkfalse;
    }
    return nktrue;
}

nkbool nkppStateConditionalOutputPassed(struct NkppState *state)
{
    // No conditionals at all. Default to outputting.
    if(!state->conditionalStack) {
        return nktrue;
    }

    // Whatever our current result is, if the outer conditional
    // failed, then this one is too.
    if(!state->conditionalStack->parentPassed) {
        return nkfalse;
    }

    return state->conditionalStack->passed;
}

nkbool nkppStatePushIfResult(
    struct NkppState *state,
    nkbool ifResult)
{
    struct NkppStateConditional *newConditional =
        nkppMalloc(state, sizeof(struct NkppStateConditional));

    if(!newConditional) {
        return nkfalse;
    }

    newConditional->parentPassed =
        nkppStateConditionalOutputPassed(state);
    newConditional->passed = ifResult;
    newConditional->passedOriginalConditional = ifResult;
    newConditional->seenElseAlready = nkfalse;
    newConditional->passedElifAlready = nkfalse;
    newConditional->next = state->conditionalStack;
    state->conditionalStack = newConditional;

    return nktrue;
}

nkbool nkppStatePopIfResult(
    struct NkppState *state)
{
    struct NkppStateConditional *stackTop = state->conditionalStack;

    if(!stackTop) {
        return nkfalse;
    }
    state->conditionalStack = stackTop->next;
    nkppFree(state, stackTop);

    return nktrue;
}

nkbool nkppStateFlipIfResultForElse(
    struct NkppState *state)
{
    struct NkppStateConditional *stackTop = state->conditionalStack;
    if(!stackTop) {
        nkppStateAddError(
            state,
            "\"else\" directive without \"if\".");
        return nkfalse;
    }

    if(stackTop->seenElseAlready) {
        nkppStateAddError(
            state,
            "Multiple \"else\" directives.");
        return nkfalse;
    }

    if(stackTop->passedOriginalConditional) {
        stackTop->passed = nkfalse;
    } else {
        stackTop->passed = !stackTop->passed;
    }
    stackTop->seenElseAlready = nktrue;
    stackTop->passedElifAlready = nktrue;

    return nktrue;
}

nkbool nkppStateFlipIfResultForElif(
    struct NkppState *state,
    nkbool result)
{
    struct NkppStateConditional *stackTop = state->conditionalStack;
    if(!stackTop) {
        nkppStateAddError(
            state,
            "\"elif\" directive without \"if\".");
        return nkfalse;
    }

    if(stackTop->seenElseAlready) {
        nkppStateAddError(
            state,
            "\"elif\" after \"else\" directives.");
        return nkfalse;
    }

    if(result &&
        !stackTop->passed &&
        !stackTop->passedElifAlready &&
        !stackTop->passedOriginalConditional)
    {
        stackTop->passed = nktrue;
        stackTop->passedElifAlready = nktrue;
    } else {
        stackTop->passed = nkfalse;
    }

    return nktrue;
}

nkbool nkppStateDirective(
    struct NkppState *state)
{
    nkbool ret = nktrue;
    struct NkppToken *directiveNameToken = NULL;
    nkuint32_t directiveLineCount = 0;
    char *line = NULL;

    if(!nkppStateInputSkipWhitespaceAndComments(state, nkfalse, nktrue)) {
        ret = nkfalse;
        goto nkppStateDirective_cleanup;
    }
    if(state->str[state->index] == '\n') {
        // Made it to the end of a line and didn't find anything.
        // Don't do anything (just like GCC).
        ret = nktrue;
        goto nkppStateDirective_cleanup;
    }

    // Make sure there's the start of a valid identifier
    // as the very next character. We don't support
    // whitespace between the hash and a directive name.
    if(!nkppIsValidIdentifierCharacter(state->str[state->index], nktrue)) {
        nkppStateAddError(state, "Invalid token in directive.\n");
        ret = nkfalse;
        goto nkppStateDirective_cleanup;
    }

    // Get the directive name.
    directiveNameToken = nkppStateInputGetNextToken(state, nktrue);
    if(!directiveNameToken) {
        nkppStateAddError(state, "Expected directive name.");
        ret = nkfalse;
        goto nkppStateDirective_cleanup;
    }

    // Check to see if this is something we
    // understand by going through the *actual*
    // list of directives.
    if(!nkppDirectiveIsValid(directiveNameToken->str)) {
        nkppStateAddError(state, "Invalid directive name.");
        ret = nkfalse;
        goto nkppStateDirective_cleanup;
    }

    // Read all the arguments for this directive.
    line = nkppStateInputReadRestOfLine(
        state, &directiveLineCount);
    if(!line) {
        ret = nkfalse;
        goto nkppStateDirective_cleanup;
    }

    // Execute the directive.
    if(nkppDirectiveHandleDirective(
            state,
            directiveNameToken->str,
            line))
    {
        // That went well.

    } else {

        // I don't know if we really need
        // to report an error here,
        // because an error would have
        // been added in nkppDirectiveHandleDirective()
        // for whatever went wrong.
        nkppStateAddError(
            state, "Bad directive.");
        ret = nkfalse;
    }

nkppStateDirective_cleanup:
    if(directiveNameToken) {
        nkppTokenDestroy(state, directiveNameToken);
    }
    if(line) {
        nkppFree(state, line);
    }

    return ret;
}

nkbool nkppStateExecute(
    struct NkppState *state,
    const char *str)
{
    nkbool ret = nktrue;
    struct NkppToken *token = NULL;

    // FIXME: Maybe make this less arbitraty.
    if(state->recursionLevel > 20) {
        nkppStateAddError(state, "Arbitrary recursion limit reached in preprocessor.");
        return nkfalse;
    }

    state->str = str;
    state->index = 0;

    while(state->str && state->str[state->index]) {

        token = nkppStateInputGetNextToken(state, nktrue);

        if(token) {

            if(state->concatenationEnabled && token->type == NK_PPTOKEN_DOUBLEHASH) {

                // Output nothing. This is the symbol concatenation
                // token, and it does its job by effectively just
                // dropping out.

            } else if(token->type == NK_PPTOKEN_HASH &&
                !state->concatenationEnabled &&
                state->tokensOnThisLine == 1)
            {
                // Directives.
                ret = nkppStateDirective(state) && ret;

            } else if(token->type == NK_PPTOKEN_HASH &&
                state->concatenationEnabled)
            {
                nkuint32_t stringificationStartIndex = state->index - 1;
                nkuint32_t stringificationEndIndex;
                struct NkppToken *nameToken = NULL;
                struct NkppMacro *macro = NULL;

                // Stringification.
                nameToken = nkppStateInputGetNextToken(state, nkfalse);
                stringificationEndIndex = state->index;

                if(nameToken &&
                    nameToken->type == NK_PPTOKEN_IDENTIFIER &&
                    (macro = nkppStateFindMacro(state, nameToken->str)) &&
                    macro->isArgumentName)
                {
                    if(!nkppMacroStringify(state, nameToken->str)) {
                        nkppStateAddError(state, "Stringification failed.");
                        ret = nkfalse;
                    }
                } else {

                    // Oh. That wasn't something to stringify. Better
                    // go back and emit all the text in between.
                    state->index = stringificationStartIndex;
                    while(state->index < stringificationEndIndex) {
                        nkppStateInputSkipChar(state, nktrue);
                    }

                }

                if(nameToken) {
                    nkppTokenDestroy(state, nameToken);
                    nameToken = NULL;
                }

            } else if(token->type == NK_PPTOKEN_IDENTIFIER) {

                // If we see the "defined" token, and we're
                // preprocessing something for an "#if" directive (an
                // expression), then we must skip the identifier in
                // parenthesis after it!
                //
                // This is really annoying, because the "defined" text
                // can come into the expression in many different
                // ways, and it looks like the only way to check to
                // see if we should leave this identifier alone is to
                // check to see if we have already emitted it in this
                // string already.
                //
                // This is a weird special case that goes against some
                // of my better judgment.
                if(state->preprocessingIfExpression &&
                    nkppStateInputGetNextToken_checkForDefinedExpression(state))
                {
                    if(!nkppStateOutputAppendString(state, token->str)) {
                        ret = nkfalse;
                    }

                } else {

                    // See if we can find a macro with this name.
                    struct NkppMacro *macro =
                        nkppStateFindMacro(
                            state, token->str);

                    if(macro) {

                        // Execute that macro.
                        if(!nkppMacroExecute(state, macro)) {
                            ret = nkfalse;
                        }

                    } else {

                        // This is an identifier, but it's not a defined
                        // macro.
                        if(!nkppStateOutputAppendString(state, token->str)) {
                            ret = nkfalse;
                        }

                    }
                }

            } else {

                // We don't know what this is. It's probably not for
                // us. Just pass it through.
                if(!nkppStateOutputAppendString(state, token->str)) {
                    ret = nkfalse;
                }

            }

            nkppTokenDestroy(state, token);
            token = NULL;

        } else {
            break;
        }

    }

    // If we didn't make it to the end of the input, then we had an
    // error. Cases where this can occur would be allocation failures
    // inside nkppStateInputGetNextToken(). From there an EOF and
    // allocation failure both return NULL.
    if(state->str[state->index] != 0) {
        ret = nkfalse;
    }

    return ret;
}

