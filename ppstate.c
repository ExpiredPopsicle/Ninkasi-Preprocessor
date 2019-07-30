#include "ppcommon.h"
#include "ppstate.h"
#include "ppmacro.h"
#include "pptoken.h"

#include <assert.h>

// MEMSAFE
struct PreprocessorState *createPreprocessorState(
    struct PreprocessorErrorState *errorState)
{
    struct PreprocessorState *ret =
        mallocWrapper(sizeof(struct PreprocessorState));

    if(ret) {
        ret->str = NULL;
        ret->index = 0;
        ret->lineNumber = 1;
        ret->outputLineNumber = 1;
        ret->output = NULL;
        ret->macros = NULL;
        ret->writePositionMarkers = nkfalse;
        ret->updateMarkers = nkfalse;
        ret->filename = strdupWrapper("<unknown>");
        ret->errorState = errorState;
        ret->nestedPassedIfs = 0;
        ret->nestedFailedIfs = 0;

        if(!ret->filename) {
            freeWrapper(ret);
            return NULL;
        }
    }

    return ret;
}

// MEMSAFE
void destroyPreprocessorState(struct PreprocessorState *state)
{
    struct PreprocessorMacro *currentMacro = state->macros;
    while(currentMacro) {
        struct PreprocessorMacro *next = currentMacro->next;
        destroyPreprocessorMacro(currentMacro);
        currentMacro = next;
    }
    freeWrapper(state->output);
    freeWrapper(state->filename);
    freeWrapper(state);
}

// This should only ever be called when we're about to output a
// newline anyway.
//
// MEMSAFE
nkbool preprocessorStateWritePositionMarker(struct PreprocessorState *state)
{
    char numberStr[128];
    char *escapedFilenameStr = escapeString(state->filename);
    nkbool ret = nktrue;

    if(!escapedFilenameStr) {
        return nkfalse;
    }

    ret &= appendString(state, "#file \"");
    ret &= appendString(state, escapedFilenameStr);
    ret &= appendString(state, "\"");

    sprintf(
        numberStr, "\n#line %ld\n",
        (long)state->lineNumber);
    ret &= appendString(state, numberStr);

    state->outputLineNumber = state->lineNumber;

    freeWrapper(escapedFilenameStr);

    return ret;
}

// MEMSAFE
nkbool appendString(struct PreprocessorState *state, const char *str)
{
    if(!str) {

        return nktrue;

    } else {

        nkuint32_t len = strlenWrapper(str);
        nkuint32_t i;

        for(i = 0; i < len; i++) {
            if(!appendChar(state, str[i])) {
                return nkfalse;
            }
        }
    }

    return nktrue;
}

// FIXME: Make MEMSAFE (overflow checks)
nkbool appendChar_real(struct PreprocessorState *state, char c)
{
    // TODO: Check overflow.
    nkuint32_t oldLen = state->output ? strlenWrapper(state->output) : 0;
    nkuint32_t newLen = oldLen + 1;
    // TODO: Check overflow.
    nkuint32_t allocLen = newLen + 1;

    char *newChunk = reallocWrapper(state->output, allocLen);
    if(!newChunk) {
        // Handle alloc fail.
        return nkfalse;
    }
    state->output = newChunk;

    state->output[oldLen] = c;
    state->output[newLen] = 0;

    return nktrue;
}

// MEMSAFE
nkbool FIXME_REMOVETHIS_writenumber(struct PreprocessorState *state, nkuint32_t n)
{
    nkuint32_t lnMask = 10000;
    nkbool ret = nktrue;

    while(!(n / lnMask)) {
        ret &= appendChar_real(state, ' ');
        n %= lnMask;
        lnMask /= 10;
    }

    while(lnMask) {
        ret &= appendChar_real(state, '0' + (n / lnMask));
        n %= lnMask;
        lnMask /= 10;
    }

    return ret;
}

// FIXME: Make MEMSAFE (Just check it. This one might be good.)
nkbool appendChar(struct PreprocessorState *state, char c)
{
    nkbool ret = nktrue;

    if(state->writePositionMarkers) {

        // If this is the first character on a line...
        if(!state->output || state->output[strlenWrapper(state->output) - 1] == '\n') {


            // FIXME: Replace this placeholder crap when we're done testing.

            // char lineNoBuf[128];
            // sprintf(lineNoBuf, "%ld", (long)state->lineNumber);

            // nkuint32_t lineNo = state->lineNumber;
            // nkuint32_t lnMask = 10000;

            {
                const char *filename = state->filename;
                nkuint32_t fnameLen = strlenWrapper(filename);
                nkuint32_t i;
                for(i = 0; i < 12; i++) {
                    if(i < fnameLen) {
                        ret &= appendChar_real(state, filename[i]);
                    } else {
                        ret &= appendChar_real(state, ' ');
                    }
                }
            }

            ret &= appendChar_real(state, ':');

            ret &= FIXME_REMOVETHIS_writenumber(state, state->lineNumber);
            ret &= FIXME_REMOVETHIS_writenumber(state, state->outputLineNumber);

            ret &= FIXME_REMOVETHIS_writenumber(state, state->nestedFailedIfs);
            ret &= FIXME_REMOVETHIS_writenumber(state, state->nestedPassedIfs);

            ret &= appendChar_real(state, ' ');
            ret &= appendChar_real(state, state->updateMarkers ? 'U' : ' ');

            ret &= appendChar_real(state, ' ');

            if(state->outputLineNumber != state->lineNumber) {
                ret &= appendChar_real(state, '-');
                ret &= appendChar_real(state, ' ');

                // Add in a position marker and set the output line
                // number.
                if(state->updateMarkers) {
                    state->outputLineNumber = state->lineNumber;
                    state->updateMarkers = nkfalse;
                    ret &= preprocessorStateWritePositionMarker(state);

                    // Send in a character we purposely won't do anything
                    // with just to pump the line-start debug info stuff.
                    ret &= appendChar(state, 0);
                }

            } else {
                ret &= appendChar_real(state, '|');
                ret &= appendChar_real(state, ' ');
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
            ret &= appendChar_real(state, c);
        }
    }

    return ret;
}

// MEMSAFE
nkbool skipChar(struct PreprocessorState *state, nkbool output)
{
    assert(state->str[state->index]);

    if(output) {
        if(!appendChar(state, state->str[state->index])) {
            return nkfalse;
        }
    }

    if(state->str && state->str[state->index] == '\n') {
        state->lineNumber++;
    }

    state->index++;

    return nktrue;
}

// MEMSAFE
nkbool skipWhitespaceAndComments(
    struct PreprocessorState *state,
    nkbool output,
    nkbool stopAtNewline)
{
    if(!state->str) {
        return nktrue;
    }

    while(state->str[state->index]) {

        // Skip comments (but output them).
        if(state->str[state->index] == '/' && state->str[state->index] == '/') {

            while(state->str[state->index] && state->str[state->index] != '\n') {
                if(!skipChar(state, output)) {
                    return nkfalse;
                }
            }

        } else if(!nkiCompilerIsWhitespace(state->str[state->index])) {

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

        if(!skipChar(state, output)) {
            return nkfalse;
        }
    }

    return nktrue;
}

// MEMSAFE
void preprocessorStateAddMacro(
    struct PreprocessorState *state,
    struct PreprocessorMacro *macro)
{
    macro->next = state->macros;
    state->macros = macro;
}

// FIXME: Make MEMSAFE
struct PreprocessorMacro *preprocessorStateFindMacro(
    struct PreprocessorState *state,
    const char *identifier)
{
    struct PreprocessorMacro *currentMacro = state->macros;

    if(!identifier) {
        return NULL;
    }

    // Find the macro.
    while(currentMacro) {
        if(!strcmpWrapper(currentMacro->identifier, identifier)) {
            break;
        }
        currentMacro = currentMacro->next;
    }

    // Dynamically add in or modify __FILE__ and __LINE__ macros.
    if(!strcmpWrapper(identifier, "__FILE__") ||
        !strcmpWrapper(identifier, "__LINE__"))
    {
        // Lazy-create the macro if it doesn't exist.
        if(!currentMacro) {

            currentMacro =
                createPreprocessorMacro();

            if(currentMacro) {
                if(preprocessorMacroSetIdentifier(
                        currentMacro, identifier))
                {
                    preprocessorStateAddMacro(
                        state, currentMacro);
                } else {
                    destroyPreprocessorMacro(currentMacro);
                    currentMacro = NULL;
                }
            }
        }

        // Update to whatever it is now.
        if(currentMacro) {
            if(!strcmpWrapper(identifier, "__FILE__")) {
                preprocessorMacroSetDefinition(currentMacro, state->filename);
            } else if(!strcmpWrapper(identifier, "__LINE__")) {
                char tmp[128];
                sprintf(tmp, "%lu", (unsigned long)state->lineNumber);
                preprocessorMacroSetDefinition(currentMacro, tmp);
            }
        }
    }

    return currentMacro;
}

// MEMSAFE
nkbool preprocessorStateDeleteMacro(
    struct PreprocessorState *state,
    const char *identifier)
{
    struct PreprocessorMacro *currentMacro = state->macros;
    struct PreprocessorMacro **lastPtr = &state->macros;

    while(currentMacro) {

        if(!strcmpWrapper(currentMacro->identifier, identifier)) {

            *lastPtr = currentMacro->next;
            currentMacro->next = NULL;

            destroyPreprocessorMacro(currentMacro);

            return nktrue;
        }

        lastPtr = &currentMacro->next;
        currentMacro = currentMacro->next;

    }

    return nkfalse;
}

// MEMSAFE
struct PreprocessorState *preprocessorStateClone(
    const struct PreprocessorState *state)
{
    struct PreprocessorState *ret = createPreprocessorState(
        state->errorState);
    struct PreprocessorMacro *currentMacro;
    struct PreprocessorMacro **macroWritePtr;

    if(!ret) {
        return NULL;
    }

    ret->str = state->str; // Non-owning copy! (The source doesn't own it either.)
    ret->index = state->index;
    ret->lineNumber = state->lineNumber;
    ret->outputLineNumber = state->outputLineNumber;
    ret->output = state->output ? strdupWrapper(state->output) : NULL;

    if(state->output && !ret->output) {
        destroyPreprocessorState(ret);
        return NULL;
    }

    // Note: We purposely don't write position markers or update
    // markers here.
    ret->errorState = state->errorState;
    if(!preprocessorStateSetFilename(ret, state->filename)) {
        destroyPreprocessorState(ret);
        return NULL;
    }

    // Note: Nested if state is not copied.

    currentMacro = state->macros;
    macroWritePtr = &ret->macros;

    while(currentMacro) {

        struct PreprocessorMacro *clonedMacro =
            preprocessorMacroClone(currentMacro);
        if(!clonedMacro) {
            destroyPreprocessorState(ret);
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

// MEMSAFE
char *readIdentifier(struct PreprocessorState *state)
{
    const char *str = state->str;
    nkuint32_t *i = &state->index;

    nkuint32_t start = *i;
    nkuint32_t end;
    nkuint32_t len;
    char *ret;

    while(nkiCompilerIsValidIdentifierCharacter(str[*i], *i == start)) {
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

        ret = mallocWrapper(memLen);
    }

    if(!ret) {
        return NULL;
    }

    memcpyWrapper(ret, str + start, len);
    ret[len] = 0;

    return ret;
}

// MEMSAFE
char *readQuotedString(struct PreprocessorState *state)
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
    if(!skipChar(state, nkfalse)) {
        return NULL;
    }

    while(str[*i]) {

        if(str[*i] == '\\') {

            backslashed = !backslashed;

        } else if(!backslashed && str[*i] == '"') {

            // Skip final quote.
            if(!skipChar(state, nkfalse)) {
                return NULL;
            }
            break;

        } else {

            backslashed = nkfalse;

        }

        if(!skipChar(state, nkfalse)) {
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

        ret = mallocWrapper(memLen);
    }

    if(!ret) {
        return NULL;
    }

    memcpyWrapper(ret, str + start, len);
    ret[len] = 0;

    return ret;
}

// FIXME: Make MEMSAFE (overflow checks)
char *readInteger(struct PreprocessorState *state)
{
    const char *str = state->str;
    nkuint32_t *i = &state->index;

    nkuint32_t start = *i;
    nkuint32_t end;
    nkuint32_t len;
    char *ret;

    while(nkiCompilerIsNumber(str[*i])) {
        (*i)++;
    }

    end = *i;

    len = end - start;

    // TODO: Check overflow.
    ret = mallocWrapper(len + 1);
    if(!ret) {
        return NULL;
    }

    memcpyWrapper(ret, str + start, len);
    ret[len] = 0;

    return ret;
}

// MEMSAFE
char *readMacroArgument(struct PreprocessorState *state)
{
    // Create a pristine state to read the arguments with.
    struct PreprocessorState *readerState = NULL;
    nkuint32_t parenLevel = 0;
    char *ret = NULL;
    struct PreprocessorToken *token = NULL;

    readerState = createPreprocessorState(
        state->errorState);
    if(!readerState) {
        return NULL;
    }

    // Copy input and position.
    readerState->index = state->index;
    readerState->str = state->str;

    // Start off with an allocated-but-empty string, because we have
    // to return something not-NULL to indicate a success.
    readerState->output = mallocWrapper(1);
    if(!readerState->output) {
        destroyPreprocessorState(readerState);
        return NULL;
    }
    readerState->output[0] = 0;

    // Skip whitespace up to the first token, but don't append
    // whitespace on this side of it.
    if(!skipWhitespaceAndComments(readerState, nkfalse, nkfalse)) {
        destroyPreprocessorState(readerState);
        return NULL;
    }

    do {
        // Skip whitespace.
        if(!skipWhitespaceAndComments(readerState, nktrue, nkfalse)) {
            destroyPreprocessorState(readerState);
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
        token = getNextToken(readerState, nktrue);

        if(token) {

            // Check for '(', ')', and ','. Do stuff with paren level.
            if(token->type == NK_PPTOKEN_OPENPAREN) {
                parenLevel++;
            } else if(token->type == NK_PPTOKEN_CLOSEPAREN) {
                parenLevel--;
            }

            // Add it.
            if(!appendString(readerState, token->str)) {
                destroyToken(token);
                destroyPreprocessorState(readerState);
                return NULL;
            }

            destroyToken(token);
        }

    } while(token);

    // Update read position in the source state.
    state->index = readerState->index;
    state->lineNumber += readerState->lineNumber - 1;

    // Pull the output off the pp state and return it.
    ret = readerState->output;
    readerState->output = NULL;
    destroyPreprocessorState(readerState);
    return ret;
}

// ----------------------------------------------------------------------

// MEMSAFE
void preprocessorStateClearOutput(struct PreprocessorState *state)
{
    freeWrapper(state->output);
    state->output = NULL;
    state->outputLineNumber = 1;
}

// FIXME: Make MEMSAFE
void preprocessorStateAddError(
    struct PreprocessorState *state,
    const char *errorMessage)
{
    if(state->errorState) {

        struct PreprocessorError *newError =
            mallocWrapper(sizeof(struct PreprocessorError));

        if(newError) {

            newError->filename = strdupWrapper(state->filename);
            newError->lineNumber = state->lineNumber;
            newError->text = strdupWrapper(errorMessage);
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

        fprintf(stderr, "%s:%ld: %s\n",
        state->filename,
        (long)state->lineNumber, errorMessage);

    }
}

// MEMSAFE
void preprocessorStateFlagFileLineMarkersForUpdate(
    struct PreprocessorState *state)
{
    state->updateMarkers = nktrue;
}

// MEMSAFE
nkbool preprocessorStateSetFilename(
    struct PreprocessorState *state,
    const char *filename)
{
    if(state->filename) {
        freeWrapper(state->filename);
        state->filename = NULL;
    }

    state->filename = strdupWrapper(filename);

    if(!state->filename) {
        return nkfalse;
    }
    return nktrue;
}

// ----------------------------------------------------------------------

// FIXME: Make MEMSAFE (overflow checks)
nkbool preprocessorStatePushIfResult(
    struct PreprocessorState *state,
    nkbool ifResult)
{
    // TODO: Check overflow on these.

    if(!ifResult) {

        state->nestedFailedIfs++;

    } else {

        if(state->nestedFailedIfs) {

            // If we're inside a failed "if" block, then we're going
            // to count this result as failed, too.
            state->nestedFailedIfs++;

        } else {

            state->nestedPassedIfs++;

        }
    }

    return nktrue;
}

// MEMSAFE
nkbool preprocessorStatePopIfResult(
    struct PreprocessorState *state)
{
    if(state->nestedFailedIfs) {
        state->nestedFailedIfs--;
    } else if(state->nestedPassedIfs) {
        state->nestedPassedIfs--;
    } else {
        preprocessorStateAddError(
            state,
            "\"endif\" directive without \"if\"");
        return nkfalse;
    }

    return nktrue;
}

// FIXME: Make MEMSAFE (overflows)
nkbool preprocessorStateFlipIfResult(
    struct PreprocessorState *state)
{
    if(!state->nestedPassedIfs && !state->nestedFailedIfs) {
        preprocessorStateAddError(
            state,
            "\"else\" directive without \"if\"");
        return nkfalse;
    }

    if(state->nestedFailedIfs == 1) {
        // TODO: Check overflow.
        state->nestedPassedIfs++;
        state->nestedFailedIfs--;
    } else if(state->nestedPassedIfs) {
        state->nestedPassedIfs--;
        state->nestedFailedIfs++;
    }

    return nktrue;
}

