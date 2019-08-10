#ifndef PPDIRECT_H
#define PPDIRECT_H

#include "ppcommon.h"

struct NkppState;

nkbool nkppDirective_undef(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_define(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_ifdef(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_ifndef(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_else(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_endif(
    struct NkppState *state,
    const char *restOfLine);

#endif // PPDIRECT_H
