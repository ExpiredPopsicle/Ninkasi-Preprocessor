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
    freeWrapper(state);
}

// This should only ever be called when we're about to output a
// newline anyway.
void preprocessorStateWritePositionMarker(struct PreprocessorState *state)
{
    if(state->writePositionMarkers &&
        state->outputLineNumber != state->lineNumber)
    {
        char numberStr[128];
        char *filenameStr = "filenametest"; // FIXME: Finish this.

        nkuint32_t orig1 = state->outputLineNumber;
        nkuint32_t orig2 = state->lineNumber;

        appendString(state, "\n#file ");
        appendString(state, filenameStr); // FIXME: Add quotes and escape.
        sprintf(
            numberStr, "\n#line %ld // %ld != %ld",
            (long)state->lineNumber,
            (long)orig1,
            (long)orig2);
        appendString(state, numberStr);

        state->outputLineNumber = state->lineNumber;
    }
}

// void appendString(struct PreprocessorState *state, const char *str)
// {
//     if(!str) {

//         return;

//     } else {

//         nkuint32_t oldLen = state->output ? strlenWrapper(state->output) : 0;
//         // TODO: Check overflow.
//         nkuint32_t newLen = oldLen + strlenWrapper(str);

//         // // Count up output newlines.
//         // nkuint32_t nlIterator = 0;
//         // nkuint32_t lenStr = strlenWrapper(str);
//         // for(nlIterator = 0; nlIterator < lenStr; nlIterator++) {
//         //     if(str[nlIterator] == '\n') {
//         //         state->outputLineNumber++;
//         //     }
//         // }

//         // TODO: Check overflow.
//         state->output = reallocWrapper(state->output, newLen + 1);

//         memcpyWrapper(state->output + oldLen, str, strlenWrapper(str));
//         state->output[newLen] = 0;
//     }
// }

// void appendChar(struct PreprocessorState *state, char c)
// {
//     char str[2] = { c, 0 };

//     // // FIXME: Relocate this?
//     // if(state->str[state->index] == '\n') {
//     //     state->outputLineNumber++;
//     //     preprocessorStateWritePositionMarker(state);
//     // }

//     appendString(state, str);
// }

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

void appendChar(struct PreprocessorState *state, char c)
{
    if(state->writePositionMarkers) {
        if(!state->output || state->output[strlenWrapper(state->output) - 1] == '\n') {

            // char lineNoBuf[128];
            // sprintf(lineNoBuf, "%ld", (long)state->lineNumber);

            nkuint32_t lineNo = state->lineNumber;
            nkuint32_t lnMask = 100000;

            while(!(lineNo / lnMask)) {
                appendChar_real(state, ' ');
                lineNo %= lnMask;
                lnMask /= 10;
            }

            while(lnMask) {
                appendChar_real(state, '0' + (lineNo / lnMask));
                lineNo %= lnMask;
                lnMask /= 10;
            }

            appendChar_real(state, ' ');
            appendChar_real(state, '|');
            appendChar_real(state, ' ');
        }
    }

    appendChar_real(state, c);
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
    // Note: We purposely don't write position markers.

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



