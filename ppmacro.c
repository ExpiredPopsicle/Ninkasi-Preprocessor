#include "ppmacro.h"

void destroyPreprocessorMacro(struct PreprocessorMacro *macro)
{
    struct PreprocessorMacroArgument *args = macro->arguments;
    while(args) {
        struct PreprocessorMacroArgument *next = args->next;
        freeWrapper(args->name);
        freeWrapper(args);
        args = next;
    }

    freeWrapper(macro->identifier);
    freeWrapper(macro->definition);
    freeWrapper(macro);
}

struct PreprocessorMacro *createPreprocessorMacro(void)
{
    struct PreprocessorMacro *ret =
        mallocWrapper(sizeof(struct PreprocessorMacro));

    if(ret) {
        ret->identifier = NULL;
        ret->definition = NULL;
        ret->arguments = NULL;
        ret->next = NULL;
    }

    return ret;
}

nkbool preprocessorMacroAddArgument(
    struct PreprocessorMacro *macro,
    const char *name)
{
    struct PreprocessorMacroArgument *arg =
        mallocWrapper(sizeof(struct PreprocessorMacroArgument));

    if(!arg) {
        return nkfalse;
    }

    arg->name = strdupWrapper(name);

    if(!arg->name) {
        freeWrapper(arg);
        return nkfalse;
    }

    arg->next = NULL;

    // This is annoying, but to keep the arguments in the correct
    // order for later, we need to go all the way to the end of the
    // list and add the new pointer there.
    {
        struct PreprocessorMacroArgument *existingArg = macro->arguments;
        if(!existingArg) {
            macro->arguments = arg;
        } else {
            while(existingArg->next) {
                existingArg = existingArg->next;
            }
            existingArg->next = arg;
        }
    }

    // arg->next = macro->arguments;
    // macro->arguments = arg;

    return nktrue;
}

nkbool preprocessorMacroSetIdentifier(
    struct PreprocessorMacro *macro,
    const char *identifier)
{
    if(macro->identifier) {
        freeWrapper(macro->identifier);
        macro->identifier = NULL;
    }

    macro->identifier = strdupWrapper(identifier);

    if(!macro->identifier) {
        return nkfalse;
    }

    return nktrue;
}

nkbool preprocessorMacroSetDefinition(
    struct PreprocessorMacro *macro,
    const char *definition)
{
    if(macro->definition) {
        freeWrapper(macro->definition);
        macro->definition = NULL;
    }

    macro->definition =
        strdupWrapper(definition ? definition : "");

    if(!macro->definition) {
        return nkfalse;
    }

    return nktrue;
}

struct PreprocessorMacro *preprocessorMacroClone(
    const struct PreprocessorMacro *macro)
{
    struct PreprocessorMacro *ret = createPreprocessorMacro();
    struct PreprocessorMacroArgument *currentArgument;
    struct PreprocessorMacroArgument **argumentWritePtr;

    ret->identifier = strdupWrapper(macro->identifier);
    ret->definition = strdupWrapper(macro->definition);

    currentArgument = macro->arguments;
    argumentWritePtr = &ret->arguments;
    while(currentArgument) {

        struct PreprocessorMacroArgument *clonedArg =
            mallocWrapper(sizeof(struct PreprocessorMacroArgument));
        clonedArg->next = NULL;
        clonedArg->name = strdupWrapper(currentArgument->name);

        *argumentWritePtr = clonedArg;
        argumentWritePtr = &clonedArg->next;

        currentArgument = currentArgument->next;
    }

    return ret;
}

