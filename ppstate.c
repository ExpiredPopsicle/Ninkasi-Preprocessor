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
        ret->nestedPassedIfs = 0;
        ret->nestedFailedIfs = 0;
        ret->memoryCallbacks = memoryCallbacks;
        ret->recursionLevel = 0;

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

    struct NkppMacro *currentMacro = state->macros;
    while(currentMacro) {
        struct NkppMacro *next = currentMacro->next;
        nkppMacroDestroy(state, currentMacro);
        currentMacro = next;
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
    ret = ret && nkppStateDebugOutputNumber(state, state->nestedFailedIfs);
    ret = ret && nkppStateDebugOutputNumber(state, state->nestedPassedIfs);

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
            ret = ret && nkppStateDebugOutputLineStart(state);

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
        if(!state->nestedFailedIfs || c == '\n') {
            ret = ret && nkppStateOutputAppendChar_real(state, c);
        }
    }

    return ret;
}

nkbool nkppStateInputSkipChar(struct NkppState *state, nkbool output)
{
    assert(state->str[state->index]);

    if(output) {
        if(!nkppStateOutputAppendChar(state, state->str[state->index])) {
            return nkfalse;
        }
    }

    if(state->str && state->str[state->index] == '\n') {
        state->lineNumber++;

        // FIXME: Remove this.
        printf("Line: %ld\n", (long)state->lineNumber);
    }

    state->index++;

    return nktrue;
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

    if(nkppIsValidIdentifierCharacter(state->str[state->index], nktrue)) {

        // Read identifiers (and directives).

        ret->str = nkppStateInputReadIdentifier(state);
        ret->type = NK_PPTOKEN_IDENTIFIER;

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

        ret->str = nkppMalloc(state, 3);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = state->str[state->index+1];
            ret->str[2] = 0;
        }
        ret->type = NK_PPTOKEN_DOUBLEHASH;

        success = success && nkppStateInputSkipChar(state, nkfalse);
        success = success && nkppStateInputSkipChar(state, nkfalse);

    } else if(state->str[state->index] == '#') {

        // Hash symbol.

        ret->str = nkppMalloc(state, 2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }
        ret->type = NK_PPTOKEN_HASH;

        success = success && nkppStateInputSkipChar(state, nkfalse);

    } else if(state->str[state->index] == ',') {

        // Comma.

        ret->str = nkppMalloc(state, 2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }
        ret->type = NK_PPTOKEN_COMMA;

        success = success && nkppStateInputSkipChar(state, nkfalse);

    } else if(state->str[state->index] == '(') {

        // Open parenthesis.

        ret->str = nkppMalloc(state, 2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }
        ret->type = NK_PPTOKEN_OPENPAREN;

        success = success && nkppStateInputSkipChar(state, nkfalse);

    } else if(state->str[state->index] == ')') {

        // Open parenthesis.

        ret->str = nkppMalloc(state, 2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }
        ret->type = NK_PPTOKEN_CLOSEPAREN;

        success = success && nkppStateInputSkipChar(state, nkfalse);

    } else if(state->str[state->index] == '>') {

        // Greater-than.

        ret->str = nkppMalloc(state, 2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }
        ret->type = NK_PPTOKEN_GREATERTHAN;

        success = success && nkppStateInputSkipChar(state, nkfalse);

    } else if(state->str[state->index] == '<') {

        // Less-than.

        ret->str = nkppMalloc(state, 2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }
        ret->type = NK_PPTOKEN_LESSTHAN;

        success = success && nkppStateInputSkipChar(state, nkfalse);

    } else {

        // Unknown.

        ret->str = nkppMalloc(state, 2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }

        success = success && nkppStateInputSkipChar(state, nkfalse);
    }

    if(!ret->str || !success) {
        nkppFree(state, ret->str);
        nkppFree(state, ret);
        ret = NULL;
    }

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
                nkppMacroSetDefinition(state, currentMacro, state->filename);
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

    currentMacro = state->macros;
    macroWritePtr = &ret->macros;

    while(currentMacro) {

        struct NkppMacro *clonedMacro =
            nkppMacroClone(state, currentMacro);
        if(!clonedMacro) {
            nkppStateDestroy(ret);
            return NULL;
        }

        *macroWritePtr = clonedMacro;
        macroWritePtr = &clonedMacro->next;

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

nkbool nkppStatePushIfResult(
    struct NkppState *state,
    nkbool ifResult)
{
    nkbool overflow = nkfalse;

    if(!ifResult) {

        NK_CHECK_OVERFLOW_UINT_ADD(
            state->nestedFailedIfs, 1,
            state->nestedFailedIfs, overflow);

    } else {

        if(state->nestedFailedIfs) {

            // If we're inside a failed "if" block, then we're going
            // to count this result as failed, too.
            NK_CHECK_OVERFLOW_UINT_ADD(
                state->nestedFailedIfs, 1,
                state->nestedFailedIfs, overflow);

        } else {

            NK_CHECK_OVERFLOW_UINT_ADD(
                state->nestedPassedIfs, 1,
                state->nestedPassedIfs, overflow);

        }
    }

    return !overflow;
}

nkbool nkppStatePopIfResult(
    struct NkppState *state)
{
    if(state->nestedFailedIfs) {
        state->nestedFailedIfs--;
    } else if(state->nestedPassedIfs) {
        state->nestedPassedIfs--;
    } else {
        nkppStateAddError(
            state,
            "\"endif\" directive without \"if\"");
        return nkfalse;
    }

    return nktrue;
}

nkbool nkppStateFlipIfResult(
    struct NkppState *state)
{
    nkbool overflow = nkfalse;

    if(!state->nestedPassedIfs && !state->nestedFailedIfs) {
        nkppStateAddError(
            state,
            "\"else\" directive without \"if\"");
        return nkfalse;
    }

    if(state->nestedFailedIfs == 1) {
        state->nestedFailedIfs--;
        NK_CHECK_OVERFLOW_UINT_ADD(
            state->nestedPassedIfs, 1,
            state->nestedPassedIfs, overflow);
    } else if(state->nestedPassedIfs) {
        state->nestedPassedIfs--;
        NK_CHECK_OVERFLOW_UINT_ADD(
            state->nestedFailedIfs, 1,
            state->nestedFailedIfs, overflow);
    }

    return !overflow;
}

nkbool nkppStateExecute(
    struct NkppState *state,
    const char *str)
{
    nkbool ret = nktrue;
    struct NkppToken *token = NULL;
    struct NkppToken *directiveNameToken = NULL;
    char *line = NULL;

    // FIXME: Maybe make this less arbitraty.
    if(state->recursionLevel > 20) {
        nkppStateAddError(state, "Arbitrary recursion limit reached.");
        return nkfalse;
    }

    state->str = str;
    state->index = 0;

    while(state->str && state->str[state->index]) {

        token = nkppStateInputGetNextToken(state, nktrue);

        if(token) {

            if(token->type == NK_PPTOKEN_DOUBLEHASH) {

                // Output nothing. This is the symbol concatenation
                // token, and it does its job by effectively just
                // dropping out.

            } else if(token->type == NK_PPTOKEN_HASH) {

                // Make sure there's the start of a valid identifier
                // as the very next character. We don't support
                // whitespace between the hash and a directive name.
                if(nkppIsValidIdentifierCharacter(state->str[state->index], nktrue)) {

                    // Get the directive name.
                    directiveNameToken = nkppStateInputGetNextToken(state, nktrue);

                    if(!directiveNameToken) {

                        ret = nkfalse;

                    } else {

                        nkuint32_t directiveLineCount = 0;

                        // Check to see if this is something we
                        // understand by going through the *actual*
                        // list of directives.
                        if(nkppDirectiveIsValid(directiveNameToken->str)) {

                            line = nkppStateInputReadRestOfLine(
                                state, &directiveLineCount);

                            if(!line) {

                                ret = nkfalse;

                            } else {

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

                                // Clean up.
                                nkppFree(state, line);
                                line = NULL;

                            }

                        } else if(!nkppMacroStringify(state, directiveNameToken->str)) {

                            ret = nkfalse;

                            // FIXME: Maybe we should change this to
                            // allow unknown directives to pass
                            // through.

                            // Not a macro to stringify and not a
                            // directive. Treat it as a bad directive
                            // and eat the rest of the input.
                            line = nkppStateInputReadRestOfLine(
                                state, &directiveLineCount);
                            nkppFree(state, line);
                            line = NULL;

                        }

                        nkppTokenDestroy(state, directiveNameToken);
                        directiveNameToken = NULL;

                    }

                } else {

                    // Some non-directive identifier came after the
                    // '#', so we're going to ignore it and just
                    // output it directly as though it's not a
                    // preprocessor directive.
                    if(!nkppStateOutputAppendString(state, token->str)) {
                        ret = nkfalse;
                    }

                }

            } else if(token->type == NK_PPTOKEN_IDENTIFIER) {

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

    return ret;
}

