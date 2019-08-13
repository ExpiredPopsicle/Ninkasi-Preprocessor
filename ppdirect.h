#ifndef PPDIRECT_H
#define PPDIRECT_H

#include "ppcommon.h"

struct NkppState;

// ----------------------------------------------------------------------
// General directive stuff

nkbool nkppDirectiveHandleDirective(
    struct NkppState *state,
    const char *directive,
    const char *restOfLine);

nkbool nkppDirectiveIsValid(
    const char *directive);

// ----------------------------------------------------------------------
// The individual directives

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
