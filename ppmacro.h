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

    struct NkppMacro *next;
};

void destroyNkppMacro(
    struct NkppState *state,
    struct NkppMacro *macro);

struct NkppMacro *createNkppMacro(
    struct NkppState *state);

nkbool preprocessorMacroAddArgument(
    struct NkppState *state,
    struct NkppMacro *macro,
    const char *name);

nkbool preprocessorMacroSetIdentifier(
    struct NkppState *state,
    struct NkppMacro *macro,
    const char *identifier);

nkbool preprocessorMacroSetDefinition(
    struct NkppState *state,
    struct NkppMacro *macro,
    const char *definition);

struct NkppMacro *preprocessorMacroClone(
    struct NkppState *state,
    const struct NkppMacro *macro);

#endif // NK_PPMACRO_H
