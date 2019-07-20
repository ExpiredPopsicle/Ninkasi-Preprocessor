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
        ret->output = NULL;
        ret->macros = NULL;
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

void appendString(struct PreprocessorState *state, const char *str)
{
    nkuint32_t oldLen = state->output ? strlenWrapper(state->output) : 0;
    // TODO: Check overflow.
    nkuint32_t newLen = oldLen + strlenWrapper(str);

    // TODO: Check overflow.
    state->output = reallocWrapper(state->output, newLen + 1);

    memcpyWrapper(state->output + oldLen, str, strlenWrapper(str));
    state->output[newLen] = 0;
}

void appendChar(struct PreprocessorState *state, char c)
{
    char str[2] = { c, 0 };
    appendString(state, str);
}

void skipChar(struct PreprocessorState *state, nkbool output)
{
    if(output) {
        appendChar(state, state->str[state->index]);
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
                state->lineNumber++;
            }
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
    ret->output = state->output ? strdupWrapper(state->output) : NULL;

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
    (*i)++;

    while(str[*i]) {

        if(str[*i] == '\\') {

            backslashed = !backslashed;

        } else if(!backslashed && str[*i] == '"') {

            // Skip final quote.
            (*i)++;
            break;

        } else {

            backslashed = nkfalse;

        }

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
