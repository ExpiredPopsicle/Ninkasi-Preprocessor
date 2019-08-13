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
    struct NkppMacro *macro,
    nkuint32_t recursionLevel);

nkbool nkppMacroStringify(
    struct NkppState *state,
    const char *macroName,
    nkuint32_t recursionLevel);

#endif // NK_PPMACRO_H
