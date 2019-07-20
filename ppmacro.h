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

    struct PreprocessorMacro *next;
};

void destroyPreprocessorMacro(struct PreprocessorMacro *macro);

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

struct PreprocessorMacro *preprocessorStateFindMacro(
    struct PreprocessorState *state,
    const char *identifier);

#endif // NK_PPMACRO_H
