#include "ppcommon.h"
#include "ppstate.h"
#include "ppmacro.h"
#include "pptoken.h"

#include <assert.h>

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
    }

    return ret;
}

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
void preprocessorStateWritePositionMarker(struct PreprocessorState *state)
{
    char numberStr[128];
    char *escapedFilenameStr = escapeString(state->filename);

    appendString(state, "#file \"");
    appendString(state, escapedFilenameStr);
    appendString(state, "\"");

    sprintf(
        numberStr, "\n#line %ld\n",
        (long)state->lineNumber);
    appendString(state, numberStr);

    state->outputLineNumber = state->lineNumber;

    freeWrapper(escapedFilenameStr);
}

void appendString(struct PreprocessorState *state, const char *str)
{
    if(!str) {

        return;

    } else {

        nkuint32_t len = strlenWrapper(str);
        nkuint32_t i;

        for(i = 0; i < len; i++) {
            appendChar(state, str[i]);
        }
    }
}

void appendChar_real(struct PreprocessorState *state, char c)
{
    // TODO: Check overflow.
    nkuint32_t oldLen = state->output ? strlenWrapper(state->output) : 0;
    nkuint32_t newLen = oldLen + 1;
    // TODO: Check overflow.
    nkuint32_t allocLen = newLen + 1;

    state->output = reallocWrapper(state->output, allocLen);
    state->output[oldLen] = c;
    state->output[newLen] = 0;
}


void FIXME_REMOVETHIS_writenumber(struct PreprocessorState *state, nkuint32_t n)
{
    nkuint32_t lnMask = 10000;

    while(!(n / lnMask)) {
        appendChar_real(state, ' ');
        n %= lnMask;
        lnMask /= 10;
    }

    while(lnMask) {
        appendChar_real(state, '0' + (n / lnMask));
        n %= lnMask;
        lnMask /= 10;
    }
}


void appendChar(struct PreprocessorState *state, char c)
{
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
                        appendChar_real(state, filename[i]);
                    } else {
                        appendChar_real(state, ' ');
                    }
                }
            }

            appendChar_real(state, ':');

            FIXME_REMOVETHIS_writenumber(state, state->lineNumber);
            FIXME_REMOVETHIS_writenumber(state, state->outputLineNumber);

            FIXME_REMOVETHIS_writenumber(state, state->nestedFailedIfs);
            FIXME_REMOVETHIS_writenumber(state, state->nestedPassedIfs);

            // while(!(lineNo / lnMask)) {
            //     appendChar_real(state, ' ');
            //     lineNo %= lnMask;
            //     lnMask /= 10;
            // }

            // while(lnMask) {
            //     appendChar_real(state, '0' + (lineNo / lnMask));
            //     lineNo %= lnMask;
            //     lnMask /= 10;
            // }

            appendChar_real(state, ' ');
            appendChar_real(state, state->updateMarkers ? 'U' : ' ');

            appendChar_real(state, ' ');

            if(state->outputLineNumber != state->lineNumber) {
                appendChar_real(state, '-');
                appendChar_real(state, ' ');

                // Add in a position marker and set the output line
                // number.
                if(state->updateMarkers) {
                    state->outputLineNumber = state->lineNumber;
                    state->updateMarkers = nkfalse;
                    preprocessorStateWritePositionMarker(state);
                }

                // Send in a character we purposely won't do anything
                // with just to pump the line-start debug info stuff.
                appendChar(state, 0);

            } else {
                appendChar_real(state, '|');
                appendChar_real(state, ' ');
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
            appendChar_real(state, c);
        }
    }
}

void skipChar(struct PreprocessorState *state, nkbool output)
{
    assert(state->str[state->index]);

    if(output) {
        appendChar(state, state->str[state->index]);
    }

    if(state->str && state->str[state->index] == '\n') {
        state->lineNumber++;
    }

    state->index++;
}

void skipWhitespaceAndComments(
    struct PreprocessorState *state,
    nkbool output,
    nkbool stopAtNewline)
{
    if(!state->str) {
        return;
    }

    while(state->str[state->index]) {

        // Skip comments (but output them).
        if(state->str[state->index] == '/' && state->str[state->index] == '/') {

            while(state->str[state->index] && state->str[state->index] != '\n') {
                skipChar(state, output);
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

        skipChar(state, output);
    }
}

void preprocessorStateAddMacro(
    struct PreprocessorState *state,
    struct PreprocessorMacro *macro)
{
    macro->next = state->macros;
    state->macros = macro;
}

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
            preprocessorMacroSetIdentifier(
                currentMacro, identifier);
            preprocessorStateAddMacro(
                state, currentMacro);
        }

        // Update to whatever it is now.
        if(!strcmpWrapper(identifier, "__FILE__")) {
            preprocessorMacroSetDefinition(currentMacro, state->filename);
        } else if(!strcmpWrapper(identifier, "__LINE__")) {
            char tmp[128];
            sprintf(tmp, "%lu", (unsigned long)state->lineNumber);
            preprocessorMacroSetDefinition(currentMacro, tmp);
        }
    }

    return currentMacro;
}

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

struct PreprocessorState *preprocessorStateClone(
    const struct PreprocessorState *state)
{
    struct PreprocessorState *ret = createPreprocessorState(
        state->errorState);
    struct PreprocessorMacro *currentMacro;
    struct PreprocessorMacro **macroWritePtr;

    ret->str = state->str; // Non-owning copy! (The source doesn't own it either.)
    ret->index = state->index;
    ret->lineNumber = state->lineNumber;
    ret->outputLineNumber = state->outputLineNumber;
    ret->output = state->output ? strdupWrapper(state->output) : NULL;
    // Note: We purposely don't write position markers or update
    // markers here.
    ret->errorState = state->errorState;
    preprocessorStateSetFilename(ret, state->filename);

    // Note: Nested if state is not copied.

    currentMacro = state->macros;
    macroWritePtr = &ret->macros;

    while(currentMacro) {

        struct PreprocessorMacro *clonedMacro =
            preprocessorMacroClone(currentMacro);

        *macroWritePtr = clonedMacro;
        macroWritePtr = &clonedMacro->next;

        currentMacro = currentMacro->next;
    }

    return ret;
}

// ----------------------------------------------------------------------
// Read functions

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

    // TODO: Check overflow.
    ret = mallocWrapper(len + 1);

    memcpyWrapper(ret, str + start, len);
    ret[len] = 0;

    return ret;
}

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
    skipChar(state, nkfalse);

    while(str[*i]) {

        if(str[*i] == '\\') {

            backslashed = !backslashed;

        } else if(!backslashed && str[*i] == '"') {

            // Skip final quote.
            skipChar(state, nkfalse);
            break;

        } else {

            backslashed = nkfalse;

        }

        skipChar(state, nkfalse);
    }

    end = *i;

    len = end - start;

    // TODO: Check overflow.
    ret = mallocWrapper(len + 1);

    memcpyWrapper(ret, str + start, len);
    ret[len] = 0;

    return ret;
}

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

    memcpyWrapper(ret, str + start, len);
    ret[len] = 0;

    return ret;
}

char *readMacroArgument(struct PreprocessorState *state)
{
    // Create a pristine state to read the arguments with.
    struct PreprocessorState *readerState = createPreprocessorState(
        state->errorState);
    nkuint32_t parenLevel = 0;

    char *ret = NULL;

    // Copy input and position.
    readerState->index = state->index;
    readerState->str = state->str;

    // Skip whitespace up to the first token, but don't append
    // whitespace on this side of it.
    skipWhitespaceAndComments(readerState, nkfalse, nkfalse);

    struct PreprocessorToken *token = NULL;
    do {
        skipWhitespaceAndComments(readerState, nktrue, nkfalse);

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

        // Check for '(', ')', and ','. Do stuff with paren level.
        if(token) {
            if(token->type == NK_PPTOKEN_OPENPAREN) {
                parenLevel++;
            } else if(token->type == NK_PPTOKEN_CLOSEPAREN) {
                parenLevel--;
            }
        }

        appendString(readerState, token->str);
        destroyToken(token);

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

void preprocessorStateClearOutput(struct PreprocessorState *state)
{
    freeWrapper(state->output);
    state->output = NULL;
    state->outputLineNumber = 1;
}

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

void preprocessorStateFlagFileLineMarkersForUpdate(
    struct PreprocessorState *state)
{
    state->updateMarkers = nktrue;
}

void preprocessorStateSetFilename(
    struct PreprocessorState *state,
    const char *filename)
{
    if(state->filename) {
        freeWrapper(state->filename);
        state->filename = NULL;
    }

    state->filename = strdupWrapper(filename);
}

// ----------------------------------------------------------------------

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

