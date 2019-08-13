#include "ppstate.h"
#include "ppmacro.h"

// MEMSAFE
void nkppMacroDestroy(
    struct NkppState *state,
    struct NkppMacro *macro)
{
    struct NkppMacroArgument *args = macro->arguments;
    while(args) {
        struct NkppMacroArgument *next = args->next;
        nkppFree(state, args->name);
        nkppFree(state, args);
        args = next;
    }

    nkppFree(state, macro->identifier);
    nkppFree(state, macro->definition);
    nkppFree(state, macro);
}

// MEMSAFE
struct NkppMacro *nkppMacroCreate(
    struct NkppState *state)
{
    struct NkppMacro *ret =
        nkppMalloc(state, sizeof(struct NkppMacro));

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
nkbool nkppMacroAddArgument(
    struct NkppState *state,
    struct NkppMacro *macro,
    const char *name)
{
    struct NkppMacroArgument *arg =
        nkppMalloc(
            state,
            sizeof(struct NkppMacroArgument));
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
        struct NkppMacroArgument *existingArg = macro->arguments;
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
nkbool nkppMacroSetIdentifier(
    struct NkppState *state,
    struct NkppMacro *macro,
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
nkbool nkppMacroSetDefinition(
    struct NkppState *state,
    struct NkppMacro *macro,
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
struct NkppMacro *nkppMacroClone(
    struct NkppState *state,
    const struct NkppMacro *macro)
{
    struct NkppMacro *ret;
    struct NkppMacroArgument *currentArgument;
    struct NkppMacroArgument **argumentWritePtr;

    ret = nkppMacroCreate(state);
    if(!ret) {
        return NULL;
    }

    ret->identifier = nkppStrdup(state, macro->identifier);
    if(!ret->identifier) {
        nkppMacroDestroy(state, ret);
        return NULL;
    }

    ret->definition = nkppStrdup(state, macro->definition);
    if(!ret->definition) {
        nkppMacroDestroy(state, ret);
        return NULL;
    }

    ret->functionStyleMacro = macro->functionStyleMacro;

    currentArgument = macro->arguments;
    argumentWritePtr = &ret->arguments;

    while(currentArgument) {

        struct NkppMacroArgument *clonedArg =
            nkppMalloc(
                state,
                sizeof(struct NkppMacroArgument));
        if(!clonedArg) {
            nkppMacroDestroy(state, ret);
            return NULL;
        }

        clonedArg->next = NULL;
        clonedArg->name = nkppStrdup(state, currentArgument->name);
        if(!clonedArg->name) {
            nkppMacroDestroy(state, ret);
            return NULL;
        }

        *argumentWritePtr = clonedArg;
        argumentWritePtr = &clonedArg->next;

        currentArgument = currentArgument->next;
    }

    return ret;
}

