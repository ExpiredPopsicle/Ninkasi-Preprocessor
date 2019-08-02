#include "ppstate.h"
#include "ppmacro.h"

// MEMSAFE
void destroyPreprocessorMacro(
    struct PreprocessorState *state,
    struct PreprocessorMacro *macro)
{
    struct PreprocessorMacroArgument *args = macro->arguments;
    while(args) {
        struct PreprocessorMacroArgument *next = args->next;
        nkppFree(state, args->name);
        nkppFree(state, args);
        args = next;
    }

    nkppFree(state, macro->identifier);
    nkppFree(state, macro->definition);
    nkppFree(state, macro);
}

// MEMSAFE
struct PreprocessorMacro *createPreprocessorMacro(
    struct PreprocessorState *state)
{
    struct PreprocessorMacro *ret =
        nkppMalloc(state, sizeof(struct PreprocessorMacro));

    if(ret) {
        ret->identifier = NULL;
        ret->definition = NULL;
        ret->arguments = NULL;
        ret->next = NULL;
        ret->functionStyleMacro = nkfalse;
    }

    return ret;
}

// MEMSAFE
nkbool preprocessorMacroAddArgument(
    struct PreprocessorState *state,
    struct PreprocessorMacro *macro,
    const char *name)
{
    struct PreprocessorMacroArgument *arg =
        nkppMalloc(
            state,
            sizeof(struct PreprocessorMacroArgument));
    if(!arg) {
        return nkfalse;
    }

    arg->name = nkppStrdup(state, name);
    if(!arg->name) {
        nkppFree(state, arg);
        return nkfalse;
    }

    arg->next = NULL;

    // This is annoying, but to keep the arguments in the correct
    // order for later, we need to go all the way to the end of the
    // list and add the new pointer there.
    {
        struct PreprocessorMacroArgument *existingArg = macro->arguments;
        if(!existingArg) {
            // First in the list.
            macro->arguments = arg;
        } else {
            // Search through the list and add to the end.
            while(existingArg->next) {
                existingArg = existingArg->next;
            }
            existingArg->next = arg;
        }
    }

    return nktrue;
}

// MEMSAFE
nkbool preprocessorMacroSetIdentifier(
    struct PreprocessorState *state,
    struct PreprocessorMacro *macro,
    const char *identifier)
{
    if(macro->identifier) {
        nkppFree(state, macro->identifier);
        macro->identifier = NULL;
    }

    macro->identifier =
        nkppStrdup(state, identifier ? identifier : "");

    if(!macro->identifier) {
        return nkfalse;
    }

    return nktrue;
}

// MEMSAFE
nkbool preprocessorMacroSetDefinition(
    struct PreprocessorState *state,
    struct PreprocessorMacro *macro,
    const char *definition)
{
    if(macro->definition) {
        nkppFree(state, macro->definition);
        macro->definition = NULL;
    }

    macro->definition =
        nkppStrdup(state, definition ? definition : "");

    if(!macro->definition) {
        return nkfalse;
    }

    return nktrue;
}

// MEMSAFE
struct PreprocessorMacro *preprocessorMacroClone(
    struct PreprocessorState *state,
    const struct PreprocessorMacro *macro)
{
    struct PreprocessorMacro *ret;
    struct PreprocessorMacroArgument *currentArgument;
    struct PreprocessorMacroArgument **argumentWritePtr;

    ret = createPreprocessorMacro(state);
    if(!ret) {
        return NULL;
    }

    ret->identifier = nkppStrdup(state, macro->identifier);
    if(!ret->identifier) {
        destroyPreprocessorMacro(state, ret);
        return NULL;
    }

    ret->definition = nkppStrdup(state, macro->definition);
    if(!ret->definition) {
        destroyPreprocessorMacro(state, ret);
        return NULL;
    }

    ret->functionStyleMacro = macro->functionStyleMacro;

    currentArgument = macro->arguments;
    argumentWritePtr = &ret->arguments;

    while(currentArgument) {

        struct PreprocessorMacroArgument *clonedArg =
            nkppMalloc(
                state,
                sizeof(struct PreprocessorMacroArgument));
        if(!clonedArg) {
            destroyPreprocessorMacro(state, ret);
            return NULL;
        }

        clonedArg->next = NULL;
        clonedArg->name = nkppStrdup(state, currentArgument->name);
        if(!clonedArg->name) {
            destroyPreprocessorMacro(state, ret);
            return NULL;
        }

        *argumentWritePtr = clonedArg;
        argumentWritePtr = &clonedArg->next;

        currentArgument = currentArgument->next;
    }

    return ret;
}

