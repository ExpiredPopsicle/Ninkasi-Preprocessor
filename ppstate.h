#ifndef NK_PPSTATE_H
#define NK_PPSTATE_H

#include "ppcommon.h"

struct PreprocessorMacro;

struct PreprocessorState
{
    const char *str;
    nkuint32_t index;
    nkuint32_t lineNumber;

    char *output;

    struct PreprocessorMacro *macros;
};

struct PreprocessorState *createPreprocessorState(void);
void destroyPreprocessorState(struct PreprocessorState *state);
void appendString(struct PreprocessorState *state, const char *str);
void appendChar(struct PreprocessorState *state, char c);
void skipChar(struct PreprocessorState *state, nkbool output);
void skipWhitespaceAndComments(
    struct PreprocessorState *state,
    nkbool output,
    nkbool stopAtNewline);

void preprocessorStateAddMacro(
    struct PreprocessorState *state,
    struct PreprocessorMacro *macro);

struct PreprocessorMacro *preprocessorStateFindMacro(
    struct PreprocessorState *state,
    const char *identifier);

nkbool preprocessorStateDeleteMacro(
    struct PreprocessorState *state,
    const char *identifier);

struct PreprocessorState *preprocessorStateClone(
    const struct PreprocessorState *state);

char *readIdentifier(struct PreprocessorState *state);

char *readQuotedString(struct PreprocessorState *state);

char *readInteger(struct PreprocessorState *state);

// TODO: Clear output.

#endif // NK_PPSTATE_H
