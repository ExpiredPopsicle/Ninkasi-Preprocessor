#ifndef PPDIRECT_H
#define PPDIRECT_H

#include "ppcommon.h"

struct PreprocessorState;

nkbool handleUndef(
    struct PreprocessorState *state,
    const char *restOfLine);

nkbool handleDefine(
    struct PreprocessorState *state,
    const char *restOfLine);

nkbool handleIfdef(
    struct PreprocessorState *state,
    const char *restOfLine);

nkbool handleIfndef(
    struct PreprocessorState *state,
    const char *restOfLine);

nkbool handleElse(
    struct PreprocessorState *state,
    const char *restOfLine);

nkbool handleEndif(
    struct PreprocessorState *state,
    const char *restOfLine);

#endif // PPDIRECT_H
