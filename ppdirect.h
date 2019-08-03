#ifndef PPDIRECT_H
#define PPDIRECT_H

#include "ppcommon.h"

struct NkppState;

nkbool handleUndef(
    struct NkppState *state,
    const char *restOfLine);

nkbool handleDefine(
    struct NkppState *state,
    const char *restOfLine);

nkbool handleIfdef(
    struct NkppState *state,
    const char *restOfLine);

nkbool handleIfndef(
    struct NkppState *state,
    const char *restOfLine);

nkbool handleElse(
    struct NkppState *state,
    const char *restOfLine);

nkbool handleEndif(
    struct NkppState *state,
    const char *restOfLine);

#endif // PPDIRECT_H
