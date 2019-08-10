#include "ppcommon.h"
#include "ppstate.h"
#include "ppmacro.h"
#include "pptoken.h"
#include "ppstring.h"

#include <assert.h>

void *nkppDefaultMallocWrapper(void *userData, nkuint32_t size)
{
    return mallocWrapper(size);
}

void nkppDefaultFreeWrapper(void *userData, void *ptr)
{
    freeWrapper(ptr);
}

void *nkppMalloc(struct NkppState *state, nkuint32_t size)
{
    if(state->memoryCallbacks) {
        return state->memoryCallbacks->mallocWrapper(
            state->memoryCallbacks->userData, size);
    }
    return nkppDefaultMallocWrapper(NULL, size);
}

void nkppFree(struct NkppState *state, void *ptr)
{
    if(state->memoryCallbacks) {
        state->memoryCallbacks->freeWrapper(
            state->memoryCallbacks->userData, ptr);
    }
    nkppDefaultFreeWrapper(NULL, ptr);
}

// MEMSAFE
struct NkppState *nkppCreateState(
    struct NkppErrorState *errorState,
    struct NkppMemoryCallbacks *memoryCallbacks)
{
    NkppMallocWrapper localMallocWrapper =
        memoryCallbacks ? memoryCallbacks->mallocWrapper : nkppDefaultMallocWrapper;
    NkppFreeWrapper localFreeWrapper =
        memoryCallbacks ? memoryCallbacks->freeWrapper : nkppDefaultFreeWrapper;
    void *userData = memoryCallbacks ? memoryCallbacks->userData : NULL;

    struct NkppState *ret =
        localMallocWrapper(userData, sizeof(struct NkppState));

    if(ret) {
        ret->str = NULL;
        ret->index = 0;
        ret->lineNumber = 1;
        ret->outputLineNumber = 1;
        ret->output = NULL;
        ret->macros = NULL;
        ret->writePositionMarkers = nkfalse;
        ret->updateMarkers = nkfalse;
        ret->errorState = errorState;
        ret->nestedPassedIfs = 0;
        ret->nestedFailedIfs = 0;
        ret->memoryCallbacks = memoryCallbacks;

        // Memory callbacks now set up. Can use normal functions that
        // require allocations.
        ret->filename = nkppStrdup(ret, "<unknown>");
        if(!ret->filename) {
            localFreeWrapper(userData, ret);
            return NULL;
        }
    }

    return ret;
}

// MEMSAFE
void nkppDestroyState(struct NkppState *state)
{
    NkppFreeWrapper localFreeWrapper =
        state->memoryCallbacks ? state->memoryCallbacks->freeWrapper : nkppDefaultFreeWrapper;
    void *userData =
        state->memoryCallbacks ? state->memoryCallbacks->userData : NULL;

    struct NkppMacro *currentMacro = state->macros;
    while(currentMacro) {
        struct NkppMacro *next = currentMacro->next;
        destroyNkppMacro(state, currentMacro);
        currentMacro = next;
    }

    localFreeWrapper(userData, state->output);
    localFreeWrapper(userData, state->filename);
    localFreeWrapper(userData, state);
}

// This should only ever be called when we're about to output a
// newline anyway.
//
// MEMSAFE
nkbool preprocessorStateWritePositionMarker(struct NkppState *state)
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

// MEMSAFE
nkbool nkppStateOutputAppendString(struct NkppState *state, const char *str)
{
    if(!str) {

        return nktrue;

    } else {

        nkuint32_t len = strlenWrapper(str);
        nkuint32_t i;

        for(i = 0; i < len; i++) {
            if(!nkppStateOutputAppendChar(state, str[i])) {
                return nkfalse;
            }
        }
    }

    return nktrue;
}

// MEMSAFE
nkbool nkppStateOutputAppendChar_real(struct NkppState *state, char c)
{
    // FIXME: We should obviously avoid reallocating and copying the
    //   entire output string every time we add a new character! This
    //   was simpler in the short term than setting up a larger buffer
    //   with a capacity to fill in before reallocating.

    nkuint32_t oldLen;
    nkuint32_t newLen;
    nkuint32_t allocLen;
    nkbool overflow = nkfalse;
    char *newChunk;

    // Calculate new size with overflow check.
    oldLen = state->output ? strlenWrapper(state->output) : 0;
    NK_CHECK_OVERFLOW_UINT_ADD(oldLen, 1, newLen, overflow);
    NK_CHECK_OVERFLOW_UINT_ADD(newLen, 1, allocLen, overflow);
    if(overflow) {
        return nkfalse;
    }

    // Reallocate chunk.
    newChunk = nkppRealloc(
        state, state->output, allocLen);
    if(!newChunk) {
        return nkfalse;
    }
    state->output = newChunk;

    // Add the new character and a null terminator.
    state->output[oldLen] = c;
    state->output[newLen] = 0;

    return nktrue;
}

// MEMSAFE
nkbool FIXME_REMOVETHIS_writenumber(struct NkppState *state, nkuint32_t n)
{
    nkuint32_t lnMask = 10000;
    nkbool ret = nktrue;

    while(!(n / lnMask)) {
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

// MEMSAFE
nkbool nkppStateOutputAppendChar(struct NkppState *state, char c)
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
                const char *filename = state->filename ? state->filename : "???";
                nkuint32_t fnameLen = strlenWrapper(filename);
                nkuint32_t i;
                for(i = 0; i < 12; i++) {
                    if(i < fnameLen) {
                        ret = ret && nkppStateOutputAppendChar_real(state, filename[i]);
                    } else {
                        ret = ret && nkppStateOutputAppendChar_real(state, ' ');
                    }
                }
            }

            ret = ret && nkppStateOutputAppendChar_real(state, ':');

            ret = ret && FIXME_REMOVETHIS_writenumber(state, state->lineNumber);
            ret = ret && FIXME_REMOVETHIS_writenumber(state, state->outputLineNumber);

            ret = ret && FIXME_REMOVETHIS_writenumber(state, state->nestedFailedIfs);
            ret = ret && FIXME_REMOVETHIS_writenumber(state, state->nestedPassedIfs);

            ret = ret && nkppStateOutputAppendChar_real(state, ' ');
            ret = ret && nkppStateOutputAppendChar_real(state, state->updateMarkers ? 'U' : ' ');

            ret = ret && nkppStateOutputAppendChar_real(state, ' ');

            if(state->outputLineNumber != state->lineNumber) {
                ret = ret && nkppStateOutputAppendChar_real(state, '-');
                ret = ret && nkppStateOutputAppendChar_real(state, ' ');

                // Add in a position marker and set the output line
                // number.
                if(state->updateMarkers) {
                    state->outputLineNumber = state->lineNumber;
                    state->updateMarkers = nkfalse;
                    ret = ret && preprocessorStateWritePositionMarker(state);

                    // Send in a character we purposely won't do anything
                    // with just to pump the line-start debug info stuff.
                    ret = ret && nkppStateOutputAppendChar(state, 0);
                }

            } else {
                ret = ret && nkppStateOutputAppendChar_real(state, '|');
                ret = ret && nkppStateOutputAppendChar_real(state, ' ');
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

// MEMSAFE
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
    }

    state->index++;

    return nktrue;
}

// MEMSAFE
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

        if(!nkppStateInputSkipChar(state, output)) {
            return nkfalse;
        }
    }

    return nktrue;
}

// MEMSAFE
void nkppStateAddMacro(
    struct NkppState *state,
    struct NkppMacro *macro)
{
    macro->next = state->macros;
    state->macros = macro;
}

// MEMSAFE
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
                createNkppMacro(state);

            if(currentMacro) {
                if(preprocessorMacroSetIdentifier(
                        state, currentMacro, identifier))
                {
                    nkppStateAddMacro(
                        state, currentMacro);
                } else {
                    destroyNkppMacro(
                        state, currentMacro);
                    currentMacro = NULL;
                }
            }
        }

        // Update to whatever it is now.
        if(currentMacro) {
            if(!strcmpWrapper(identifier, "__FILE__")) {
                preprocessorMacroSetDefinition(state, currentMacro, state->filename);
            } else if(!strcmpWrapper(identifier, "__LINE__")) {
                char tmp[128];
                sprintf(tmp, "%lu", (unsigned long)state->lineNumber);
                preprocessorMacroSetDefinition(state, currentMacro, tmp);
            }
        }
    }

    return currentMacro;
}

// MEMSAFE
nkbool nkppStateDeleteMacro(
    struct NkppState *state,
    const char *identifier)
{
    struct NkppMacro *currentMacro = state->macros;
    struct NkppMacro **lastPtr = &state->macros;

    while(currentMacro) {

        if(!strcmpWrapper(currentMacro->identifier, identifier)) {

            *lastPtr = currentMacro->next;
            currentMacro->next = NULL;

            destroyNkppMacro(
                state, currentMacro);

            return nktrue;
        }

        lastPtr = &currentMacro->next;
        currentMacro = currentMacro->next;

    }

    return nkfalse;
}

// MEMSAFE
struct NkppState *nkppCloneState(
    struct NkppState *state)
{
    struct NkppState *ret = nkppCreateState(
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
    ret->output = state->output ? nkppStrdup(state, state->output) : NULL;

    if(state->output && !ret->output) {
        nkppDestroyState(ret);
        return NULL;
    }

    // Note: We purposely don't write position markers or update
    // markers here.
    ret->errorState = state->errorState;
    if(!nkppStateSetFilename(ret, state->filename)) {
        nkppDestroyState(ret);
        return NULL;
    }

    // Note: Nested if state is not copied.

    currentMacro = state->macros;
    macroWritePtr = &ret->macros;

    while(currentMacro) {

        struct NkppMacro *clonedMacro =
            preprocessorMacroClone(state, currentMacro);
        if(!clonedMacro) {
            nkppDestroyState(ret);
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
char *nkppStateInputReadIdentifier(struct NkppState *state)
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

        ret = nkppMalloc(state, memLen);
    }

    if(!ret) {
        return NULL;
    }

    memcpyWrapper(ret, str + start, len);
    ret[len] = 0;

    return ret;
}

// MEMSAFE
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

    memcpyWrapper(ret, str + start, len);
    ret[len] = 0;

    return ret;
}

// MEMSAFE
char *nkppStateInputReadInteger(struct NkppState *state)
{
    const char *str = state->str;
    nkuint32_t start = state->index;
    nkuint32_t end;
    nkuint32_t len;
    nkuint32_t bufLen;
    char *ret;
    nkbool overflow = nkfalse;

    while(nkiCompilerIsNumber(str[state->index])) {
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
    memcpyWrapper(ret, str + start, len);
    ret[len] = 0;

    return ret;
}

// MEMSAFE
char *nkppStateInputReadMacroArgument(struct NkppState *state)
{
    // Create a pristine state to read the arguments with.
    struct NkppState *readerState = NULL;
    nkuint32_t parenLevel = 0;
    char *ret = NULL;
    struct NkppToken *token = NULL;

    readerState = nkppCreateState(
        state->errorState, state->memoryCallbacks);
    if(!readerState) {
        return NULL;
    }

    // Copy input and position.
    readerState->index = state->index;
    readerState->str = state->str;

    // Start off with an allocated-but-empty string, because we have
    // to return something not-NULL to indicate a success.
    readerState->output = nkppMalloc(state, 1);
    if(!readerState->output) {
        nkppDestroyState(readerState);
        return NULL;
    }
    readerState->output[0] = 0;

    // Skip whitespace up to the first token, but don't append
    // whitespace on this side of it.
    if(!nkppStateInputSkipWhitespaceAndComments(readerState, nkfalse, nkfalse)) {
        nkppDestroyState(readerState);
        return NULL;
    }

    do {
        // Skip whitespace.
        if(!nkppStateInputSkipWhitespaceAndComments(readerState, nktrue, nkfalse)) {
            nkppDestroyState(readerState);
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
        token = nkppGetNextToken(readerState, nktrue);

        if(token) {

            // Check for '(', ')', and ','. Do stuff with paren level.
            if(token->type == NK_PPTOKEN_OPENPAREN) {
                parenLevel++;
            } else if(token->type == NK_PPTOKEN_CLOSEPAREN) {
                parenLevel--;
            }

            // Add it.
            if(!nkppStateOutputAppendString(readerState, token->str)) {
                destroyToken(state, token);
                nkppDestroyState(readerState);
                return NULL;
            }

            destroyToken(state, token);
        }

    } while(token);

    // Update read position in the source state.
    state->index = readerState->index;
    state->lineNumber += readerState->lineNumber - 1;

    // Pull the output off the pp state and return it.
    ret = readerState->output;
    readerState->output = NULL;
    nkppDestroyState(readerState);
    return ret;
}

// ----------------------------------------------------------------------

// MEMSAFE
void nkppStateOutputClear(struct NkppState *state)
{
    nkppFree(state, state->output);
    state->output = NULL;
    state->outputLineNumber = 1;
}

// MEMSAFE
void preprocessorStateAddError(
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

        fprintf(stderr, "%s:%ld: %s\n",
        state->filename,
        (long)state->lineNumber, errorMessage);

    }
}

// MEMSAFE
void nkppStateFlagFileLineMarkersForUpdate(
    struct NkppState *state)
{
    state->updateMarkers = nktrue;
}

// MEMSAFE
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

// ----------------------------------------------------------------------

// MEMSAFE
nkbool preprocessorStatePushIfResult(
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

// MEMSAFE
nkbool preprocessorStatePopIfResult(
    struct NkppState *state)
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

// MEMSAFE
nkbool preprocessorStateFlipIfResult(
    struct NkppState *state)
{
    nkbool overflow = nkfalse;

    if(!state->nestedPassedIfs && !state->nestedFailedIfs) {
        preprocessorStateAddError(
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

