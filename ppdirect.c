#include "ppstate.h"
#include "pptoken.h"
#include "ppdirect.h"
#include "ppmacro.h"
#include "ppstring.h"

// MEMSAFE
nkbool nkppDirective_ifdefReal(
    struct NkppState *state,
    const char *restOfLine,
    nkbool invert)
{
    nkbool ret = nkfalse;
    struct NkppState *directiveParseState =
        nkppCreateState(state->errorState, state->memoryCallbacks);
    struct NkppToken *identifierToken = NULL;
    struct NkppMacro *macro = NULL;

    if(directiveParseState) {

        directiveParseState->str = restOfLine;

        // Get identifier.
        identifierToken =
            nkppStateInputGetNextToken(directiveParseState, nkfalse);

        if(identifierToken) {

            if(identifierToken->type == NK_PPTOKEN_IDENTIFIER) {

                macro = nkppStateFindMacro(
                    state, identifierToken->str);

                if(macro) {
                    nkppStatePushIfResult(state, !invert);
                } else {
                    nkppStatePushIfResult(state, invert);
                }

                ret = nktrue;

            } else {

                // That's not an identifier.
                nkppStateAddError(state, "Expected identifier after #ifdef/ifndef.");
            }
        }
    }

    if(identifierToken) {
        destroyToken(state, identifierToken);
    }
    if(directiveParseState) {
        nkppDestroyState(directiveParseState);
    }
    return ret;
}

// MEMSAFE
nkbool nkppDirective_ifdef(
    struct NkppState *state,
    const char *restOfLine)
{
    return nkppDirective_ifdefReal(state, restOfLine, nkfalse);
}

// MEMSAFE
nkbool nkppDirective_ifndef(
    struct NkppState *state,
    const char *restOfLine)
{
    return nkppDirective_ifdefReal(state, restOfLine, nktrue);
}

// MEMSAFE
nkbool nkppDirective_else(
    struct NkppState *state,
    const char *restOfLine)
{
    return nkppStateFlipIfResult(state);
}

// MEMSAFE
nkbool nkppDirective_endif(
    struct NkppState *state,
    const char *restOfLine)
{
    return nkppStatePopIfResult(state);
}

// MEMSAFE
nkbool nkppDirective_undef(
    struct NkppState *state,
    const char *restOfLine)
{
    nkbool ret = nktrue;
    struct NkppState *directiveParseState = NULL;
    struct NkppToken *identifierToken = NULL;

    directiveParseState =
        nkppCreateState(state->errorState, state->memoryCallbacks);
    if(!directiveParseState) {
        return nkfalse;
    }

    directiveParseState->str = restOfLine;

    // Get identifier.
    identifierToken = nkppStateInputGetNextToken(directiveParseState, nkfalse);
    if(!identifierToken) {
        nkppDestroyState(directiveParseState);
        return nkfalse;
    }

    if(identifierToken->type == NK_PPTOKEN_IDENTIFIER) {

        if(nkppStateDeleteMacro(state, identifierToken->str)) {

            // Success!

        } else {

            nkppStateAddError(state, "Cannot delete macro.");
            ret = nkfalse;

        }

    } else {

        nkppStateAddError(state, "Invalid identifier.");
        ret = nkfalse;

    }

    destroyToken(state, identifierToken);
    nkppDestroyState(directiveParseState);
    return ret;
}

// MEMSAFE
nkbool nkppDirective_define(
    struct NkppState *state,
    const char *restOfLine)
{
    nkbool ret = nktrue;
    struct NkppState *directiveParseState = NULL;
    struct NkppMacro *macro = NULL;
    struct NkppToken *identifierToken = NULL;
    char *definition = NULL;
    nkbool expectingComma = nkfalse;
    struct NkppToken *openParenToken = NULL;
    struct NkppToken *argToken = NULL;

    // Setup pp state.
    directiveParseState = nkppCreateState(
        state->errorState, state->memoryCallbacks);
    if(!directiveParseState) {
        ret = nkfalse;
        goto nkppDirective_define_cleanup;
    }
    directiveParseState->str = restOfLine;

    // Setup new macro.
    macro = nkppMacroCreate(state);
    if(!macro) {
        ret = nkfalse;
        goto nkppDirective_define_cleanup;
    }

    // Get identifier.
    identifierToken =
        nkppStateInputGetNextToken(directiveParseState, nkfalse);
    if(!identifierToken) {
        ret = nkfalse;
        goto nkppDirective_define_cleanup;
    }

    // That better be an identifier.
    if(identifierToken->type != NK_PPTOKEN_IDENTIFIER) {
        nkppStateAddError(state, "Expected identifier after #define.");
        ret = nkfalse;
        goto nkppDirective_define_cleanup;
    }

    if(!nkppMacroSetIdentifier(
            state, macro, identifierToken->str))
    {
        ret = nkfalse;
        goto nkppDirective_define_cleanup;
    }

    if(!nkppStateInputSkipWhitespaceAndComments(
            directiveParseState, nkfalse, nkfalse))
    {
        ret = nkfalse;
        goto nkppDirective_define_cleanup;
    }

    // Check for arguments (next char == '(').
    if(directiveParseState->str[directiveParseState->index] == '(') {

        // Skip the open paren.
        openParenToken =
            nkppStateInputGetNextToken(directiveParseState, nkfalse);
        if(!openParenToken) {
            ret = nkfalse;
            goto nkppDirective_define_cleanup;
        }

        destroyToken(state, openParenToken);
        openParenToken = NULL;

        // Parse the argument list.

        argToken = nkppStateInputGetNextToken(directiveParseState, nkfalse);

        macro->functionStyleMacro = nktrue;

        if(!argToken) {

            // We're done, but with an error!
            nkppStateAddError(
                state, "Ran out of tokens while parsing argument list.");
            ret = nkfalse;

        } else if(argToken->type == NK_PPTOKEN_CLOSEPAREN) {

            // Zero-argument list. Nothing to do here.
            destroyToken(state, argToken);
            argToken = NULL;

        } else {

            // Argument list with actual arguments detected.

            while(1) {

                if(expectingComma) {

                    if(argToken->type == NK_PPTOKEN_CLOSEPAREN) {

                        // We're done. Bail out.
                        destroyToken(state, argToken);
                        argToken = NULL;
                        break;

                    } else if(argToken->type == NK_PPTOKEN_COMMA) {

                        // Okay. This is what we expect, but
                        // there's nothing useful here.
                        expectingComma = nkfalse;

                    } else {

                        // Error.
                        ret = nkfalse;
                    }

                } else {

                    if(argToken->type == NK_PPTOKEN_IDENTIFIER) {

                        // Valid identifier.
                        if(!nkppMacroAddArgument(
                                state, macro, argToken->str))
                        {
                            ret = nkfalse;
                        }
                        expectingComma = nktrue;

                    } else {

                        // Not a valid identifier.
                        ret = nkfalse;
                        destroyToken(state, argToken);
                        argToken = NULL;
                        break;

                    }
                }

                // Next token.
                destroyToken(state, argToken);
                argToken = nkppStateInputGetNextToken(directiveParseState, nkfalse);

                // We're not supposed to hit the end of the
                // list here.
                if(!argToken) {
                    nkppStateAddError(state, "Incomplete argument list in #define.");
                    ret = nkfalse;
                    break;
                }
            }
        }

        // Skip up to the actual definition.
        if(!nkppStateInputSkipWhitespaceAndComments(directiveParseState, nkfalse, nkfalse)) {
            ret = nkfalse;
            goto nkppDirective_define_cleanup;
        }
    }

    // Read the macro definition.
    definition = nkppStripCommentsAndTrim(
        state,
        directiveParseState->str + directiveParseState->index);
    if(!definition) {
        ret = nkfalse;
        goto nkppDirective_define_cleanup;
    }

    // Set it.
    if(!nkppMacroSetDefinition(
            state, macro, definition))
    {
        ret = nkfalse;
        goto nkppDirective_define_cleanup;
    }

    // Disallow multiple definitions.
    if(nkppStateFindMacro(state, macro->identifier)) {
        nkppStateAddError(state, "Multiple definitions of the same macro.");
        ret = nkfalse;
    }

    // Add definition to list, or clean up if we had an error.
    if(ret) {
        nkppStateAddMacro(state, macro);
        macro = NULL;
    } else {
        nkppMacroDestroy(state, macro);
        macro = NULL;
    }

nkppDirective_define_cleanup:

    // Cleanup.
    if(directiveParseState) {
        nkppDestroyState(directiveParseState);
    }
    if(identifierToken) {
        destroyToken(state, identifierToken);
    }
    if(definition) {
        nkppFree(state, definition);
    }
    if(openParenToken) {
        destroyToken(state, openParenToken);
    }
    if(macro) {
        nkppMacroDestroy(state, macro);
    }

    return ret;
}
