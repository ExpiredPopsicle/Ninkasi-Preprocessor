#include "ppcommon.h"
#include "ppstate.h"
#include "ppmacro.h"
#include "pptoken.h"

#include <assert.h>

struct PreprocessorState *createPreprocessorState(void)
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
    const char *filenameStr = state->filename;
    nkuint32_t bufSize = strlen(filenameStr) * 2 + 3;
    char *escapedFilenameStr = mallocWrapper(bufSize);

    escapedFilenameStr[0] = 0;
    nkiDbgAppendEscaped(bufSize, escapedFilenameStr, filenameStr);

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
        appendChar_real(state, c);
    }
}

void skipChar(struct PreprocessorState *state, nkbool output)
{
    assert(state->str[state->index]);

    if(output) {

        // // FIXME: Relocate this?
        // if(state->str[state->index] == '\n') {
        //     // state->outputLineNumber++;
        //     preprocessorStateWritePositionMarker(state);
        // }

        appendChar(state, state->str[state->index]);

        // // FIXME: Relocate?
        // if(state->str[state->index] == '\n') {
        //     state->outputLineNumber++;
        //     preprocessorStateWritePositionMarker(state);
        // }

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
            } else {
                // state->lineNumber++;
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

    while(currentMacro) {

        if(!strcmpWrapper(currentMacro->identifier, identifier)) {
            return currentMacro;
        }

        currentMacro = currentMacro->next;
    }

    return NULL;
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
    struct PreprocessorState *ret = createPreprocessorState();
    struct PreprocessorMacro *currentMacro;
    struct PreprocessorMacro **macroWritePtr;

    ret->str = state->str; // Non-owning copy! (The source doesn't own it either.)
    ret->index = state->index;
    ret->lineNumber = state->lineNumber;
    ret->outputLineNumber = state->outputLineNumber;
    ret->output = state->output ? strdupWrapper(state->output) : NULL;
    // Note: We purposely don't write position markers or update
    // markers here.
    preprocessorStateSetFilename(ret, state->filename);

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
    struct PreprocessorState *readerState = createPreprocessorState();
    nkuint32_t parenLevel = 0;

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

    {
        char *ret = readerState->output;
        readerState->output = NULL;
        destroyPreprocessorState(readerState);
        return ret;
    }
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
    // FIXME: Add filename.
    printf("ERROR %s:%ld: %s\n",
        state->filename,
        (long)state->lineNumber, errorMessage);
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
