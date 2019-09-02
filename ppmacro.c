#include "ppcommon.h"

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
        ret->isArgumentName = nkfalse;
    }

    return ret;
}

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

struct NkppMacro *nkppMacroClone(
    struct NkppState *state,
    const struct NkppMacro *macro)
{
    struct NkppMacro *ret;
    struct NkppMacroArgument *currentArgument;
    struct NkppMacroArgument **argumentWritePtr;

    ret = nkppMacroCreate(state);
    if(!ret) {

        // FIXME: Remove this.
        assert(0);

        return NULL;
    }

    // FIXME: Remove this.
    assert(macro->identifier);

    ret->identifier = nkppStrdup(state, macro->identifier);
    if(!ret->identifier) {
        nkppMacroDestroy(state, ret);

        // FIXME: Remove this.
        assert(!state->errorState->allocationFailure);

        // FIXME: Remove this.
        assert(0);

        return NULL;
    }

    ret->definition = nkppStrdup(state, macro->definition);
    if(!ret->definition) {

        // FIXME: Remove this.
        assert(0);

        nkppMacroDestroy(state, ret);
        return NULL;
    }

    ret->functionStyleMacro = macro->functionStyleMacro;
    ret->isArgumentName = macro->isArgumentName;

    currentArgument = macro->arguments;
    argumentWritePtr = &ret->arguments;

    while(currentArgument) {

        struct NkppMacroArgument *clonedArg =
            nkppMalloc(
                state,
                sizeof(struct NkppMacroArgument));
        if(!clonedArg) {
            nkppMacroDestroy(state, ret);

            // FIXME: Remove this.
            assert(0);

            return NULL;
        }

        clonedArg->next = NULL;
        clonedArg->name = nkppStrdup(state, currentArgument->name);
        if(!clonedArg->name) {
            nkppFree(state, clonedArg);
            nkppMacroDestroy(state, ret);

            // FIXME: Remove this.
            assert(0);

            return NULL;
        }

        *argumentWritePtr = clonedArg;
        argumentWritePtr = &clonedArg->next;

        currentArgument = currentArgument->next;
    }

    // FIXME: Remove this.
    assert(ret);

    return ret;
}

nkbool nkppMacroExecute(
    struct NkppState *state,
    struct NkppMacro *macro)
{
    struct NkppState *clonedState;
    nkbool ret = nktrue;
    char *unstrippedArgumentText = NULL;
    char *argumentText = NULL;
    struct NkppMacro *newMacro = NULL;

    // FIXME: Remove this.
    printf("FFFF Start ----------\n");

    clonedState = nkppStateClone(state, nkfalse, nkfalse);
    if(!clonedState) {
        return nkfalse;
    }
    clonedState->concatenationEnabled = nktrue;

    // Input is the macro definition. Output is
    // appending to the "parent" state.

    if(macro->arguments || macro->functionStyleMacro) {

        if(!nkppStateInputSkipWhitespaceAndComments(state, nkfalse, nkfalse)) {
            ret = nkfalse;
            goto nkppMacroExecute_cleanup;
        }

        if(state->str[state->index] == '(') {

            struct NkppMacroArgument *argument = macro->arguments;

            // Skip open paren.
            if(!nkppStateInputSkipChar(state, nkfalse)) {
                ret = nkfalse;
                goto nkppMacroExecute_cleanup;
            }

            while(argument) {

                // Read the macro argument.
                unstrippedArgumentText = nkppStateInputReadMacroArgument(state);
                if(!unstrippedArgumentText) {
                    ret = nkfalse;
                    goto nkppMacroExecute_cleanup;
                }

                argumentText = nkppStripCommentsAndTrim(
                    state, unstrippedArgumentText);
                if(!argumentText) {
                    ret = nkfalse;
                    goto nkppMacroExecute_cleanup;
                }

                nkppFree(state, unstrippedArgumentText);
                unstrippedArgumentText = NULL;

                // FIXME!!! The macro arguments need to be
                // preprocessed so we pass the right text through to
                // the next level.
                {
                    // FIXME!!! Check errors on all this crap.
                    struct NkppState *clonedState =
                        nkppStateClone(state, nkfalse, nktrue);

                    printf("FFFF Argument text (%p) (before): %s = %s\n", clonedState, argument->name, argumentText);

                    clonedState->concatenationEnabled = nktrue;
                    nkppStateExecute_internal(
                        clonedState,
                        argumentText);

                    nkppFree(state, argumentText);
                    argumentText = nkppStrdup(state, clonedState->output);

                    nkppStateDestroy_internal(clonedState);
                    printf("FFFF Argument text (%p): %s = %s\n", clonedState, argument->name, argumentText);
                }


                // Add the argument as a macro to
                // the new cloned state.
                {
                    newMacro = nkppMacroCreate(state);
                    if(!newMacro) {
                        ret = nkfalse;
                        goto nkppMacroExecute_cleanup;
                    }

                    if(!nkppMacroSetIdentifier(state, newMacro, argument->name)) {
                        ret = nkfalse;
                        goto nkppMacroExecute_cleanup;
                    }

                    if(!nkppMacroSetDefinition(state, newMacro, argumentText)) {
                        ret = nkfalse;
                        goto nkppMacroExecute_cleanup;
                    }

                    newMacro->isArgumentName = nktrue;

                    nkppStateAddMacro(clonedState, newMacro);
                    newMacro = NULL;
                }

                nkppFree(state, argumentText);
                argumentText = NULL;

                if(!nkppStateInputSkipWhitespaceAndComments(state, nkfalse, nkfalse)) {
                    ret = nkfalse;
                    goto nkppMacroExecute_cleanup;
                }

                if(argument->next) {

                    // Expect ','
                    if(state->str[state->index] == ',') {
                        if(!nkppStateInputSkipChar(state, nkfalse)) {
                            ret = nkfalse;
                            goto nkppMacroExecute_cleanup;
                        }
                    } else {
                        nkppStateAddError(state, "Expected ','.");
                        ret = nkfalse;
                        break;
                    }

                } else {

                    // Expect ')'
                    if(state->str[state->index] != ')') {
                        nkppStateAddError(state, "Expected ')'.");
                        ret = nkfalse;
                        break;
                    }
                }

                argument = argument->next;
            }

            // Skip final ')'.
            if(state->str[state->index] == ')') {
                if(!nkppStateInputSkipChar(state, nkfalse)) {
                    ret = nkfalse;
                    goto nkppMacroExecute_cleanup;
                }
            } else {
                nkppStateAddError(state, "Expected ')'.");
                ret = nkfalse;
            }

        } else {
            nkppStateAddError(state, "Expected argument list.");

            // FIXME: Remove this.
            nkppStateAddError(state, state->str);

            ret = nkfalse;
        }

    } else {

        // No arguments. Nothing to set up on cloned
        // state.

    }

    // FIXME: Remove this.
    printf("FFFF End ----------\n");

    // Preprocess the macro into place.
    if(ret) {

        // FIXME: Remove this.
        {
            struct NkppMacroArgument *arg = macro->arguments;
            struct NkppMacro *mac = clonedState->macros;
            printf("FFFF Executing macro: %s = %s\n", macro->identifier, macro->definition);
            while(arg) {
                printf("FFFF   Argument: %s\n", arg->name);
                arg = arg->next;
            }
            while(mac) {
                printf("FFFF   Inner macro: %s = %s\n", mac->identifier, mac->definition);
                mac = mac->next;
            }
            printf("FFFF   Text (%p): %s\n", clonedState, macro->definition);
        }


        // Feed the macro definition through the cloned state.
        if(!nkppStateExecute_internal(
                clonedState,
                macro->definition ? macro->definition : ""))
        {
            ret = nkfalse;
        }

        // FIXME: Remove this.
        {
            printf("FFFF  Result (%p): %s\n", clonedState, clonedState->output);
        }

        // Write output.
        if(clonedState->output) {
            if(!nkppStateOutputAppendString(state, clonedState->output)) {
                ret = nkfalse;
                goto nkppMacroExecute_cleanup;
            }
        }

        nkppStateFlagFileLineMarkersForUpdate(state);
    }

nkppMacroExecute_cleanup:

    // Clean up.
    if(clonedState) {
        nkppStateDestroy_internal(clonedState);
    }
    if(unstrippedArgumentText) {
        nkppFree(state, unstrippedArgumentText);
    }
    if(argumentText) {
        nkppFree(state, argumentText);
    }
    if(newMacro) {
        nkppMacroDestroy(state, newMacro);
    }

    return ret;
}

nkbool nkppMacroStringify(
    struct NkppState *state,
    const char *macroName)
{
    nkbool ret = nkfalse;
    char *escapedStr = NULL;
    struct NkppState *macroState = NULL;
    struct NkppMacro *macro =
        nkppStateFindMacro(
            state, macroName);

    if(macro) {

        macroState = nkppStateClone(state, nkfalse, nkfalse);

        if(macroState) {

            if(nkppMacroExecute(macroState, macro)) {

                // Escape the string and add it to the output.
                escapedStr = nkppEscapeString(
                    state, macroState->output);
                if(escapedStr) {
                    nkppStateOutputAppendString(state, "\"");
                    nkppStateOutputAppendString(state, escapedStr);
                    nkppStateOutputAppendString(state, "\"");
                    nkppFree(state, escapedStr);
                }

                // Skip past the stuff we read in the
                // cloned state.
                state->index = macroState->index;

                ret = nktrue;
            }

            nkppStateDestroy_internal(macroState);
        }

    } else {
        nkppStateAddError(
            state,
            "Unknown input for stringification.");
    }

    return ret;
}


