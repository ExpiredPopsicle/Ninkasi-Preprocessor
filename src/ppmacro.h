#ifndef NK_PPMACRO_H
#define NK_PPMACRO_H

#include "ppcommon.h"

struct NkppState;

struct NkppMacroArgument
{
    char *name;
    struct NkppMacroArgument *next;
};

struct NkppMacro
{
    char *identifier;
    char *definition;
    struct NkppMacroArgument *arguments;

    // Normally we'd just use arguments == NULL to see if this isn't a
    // function style macro, but if we have zero parameters, it'll be
    // NULL and we need this instead.
    nkbool functionStyleMacro;

    // This is true if this macro is part of the parameter list being
    // used in an outer macro. For example, the 'x' in foo(x). These
    // are NOT carried on to child states, and stringification only
    // works with them.
    nkbool isArgumentName;

    // If this is true, then this macro does not own any of the
    // strings or arguments attached to it.
    nkbool isClone;

    struct NkppMacro *next;
};

void nkppMacroDestroy(
    struct NkppState *state,
    struct NkppMacro *macro);

struct NkppMacro *nkppMacroCreate(
    struct NkppState *state);

nkbool nkppMacroAddArgument(
    struct NkppState *state,
    struct NkppMacro *macro,
    const char *name);

nkbool nkppMacroSetIdentifier(
    struct NkppState *state,
    struct NkppMacro *macro,
    const char *identifier);

nkbool nkppMacroSetDefinition(
    struct NkppState *state,
    struct NkppMacro *macro,
    const char *definition);

struct NkppMacro *nkppMacroClone(
    struct NkppState *state,
    const struct NkppMacro *macro);

nkbool nkppMacroExecute(
    struct NkppState *state,
    struct NkppMacro *macro);

nkbool nkppMacroStringify(
    struct NkppState *state,
    const char *macroName);

#endif // NK_PPMACRO_H
