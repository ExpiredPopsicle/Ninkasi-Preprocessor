#ifndef NK_PPMACRO_H
#define NK_PPMACRO_H

#include "ppcommon.h"

struct PreprocessorState;

struct PreprocessorMacroArgument
{
    char *name;
    struct PreprocessorMacroArgument *next;
};

struct PreprocessorMacro
{
    char *identifier;
    char *definition;
    struct PreprocessorMacroArgument *arguments;

    // Normally we'd just use arguments == NULL to see if this isn't a
    // function style macro, but if we have zero parameters, it'll be
    // NULL and we need this instead.
    nkbool functionStyleMacro;

    struct PreprocessorMacro *next;
};

void destroyPreprocessorMacro(
    struct PreprocessorMacro *macro);

struct PreprocessorMacro *createPreprocessorMacro(void);

nkbool preprocessorMacroAddArgument(
    struct PreprocessorMacro *macro,
    const char *name);

nkbool preprocessorMacroSetIdentifier(
    struct PreprocessorMacro *macro,
    const char *identifier);

nkbool preprocessorMacroSetDefinition(
    struct PreprocessorMacro *macro,
    const char *definition);

struct PreprocessorMacro *preprocessorMacroClone(
    const struct PreprocessorMacro *macro);

#endif // NK_PPMACRO_H
