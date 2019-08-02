#include "ppstate.h"
#include "pptoken.h"
#include "ppdirect.h"
#include "ppmacro.h"
#include "ppstring.h"

// MEMSAFE
nkbool handleIfdefReal(
    struct PreprocessorState *state,
    const char *restOfLine,
    nkbool invert)
{
    nkbool ret = nkfalse;
    struct PreprocessorState *directiveParseState =
        createPreprocessorState(state->errorState, state->memoryCallbacks);
    struct PreprocessorToken *identifierToken = NULL;
    struct PreprocessorMacro *macro = NULL;

    if(directiveParseState) {

        directiveParseState->str = restOfLine;

        // Get identifier.
        identifierToken =
            getNextToken(directiveParseState, nkfalse);

        if(identifierToken) {

            if(identifierToken->type == NK_PPTOKEN_IDENTIFIER) {

                macro = preprocessorStateFindMacro(
                    state, identifierToken->str);

                if(macro) {
                    preprocessorStatePushIfResult(state, !invert);
                } else {
                    preprocessorStatePushIfResult(state, invert);
                }

                ret = nktrue;

            } else {

                // That's not an identifier.
                preprocessorStateAddError(state, "Expected identifier after #ifdef/ifndef.");
            }
        }
    }

    if(identifierToken) {
        destroyToken(state, identifierToken);
    }
    if(directiveParseState) {
        destroyPreprocessorState(directiveParseState);
    }
    return ret;
}

// MEMSAFE
nkbool handleIfdef(
    struct PreprocessorState *state,
    const char *restOfLine)
{
    return handleIfdefReal(state, restOfLine, nkfalse);
}

// MEMSAFE
nkbool handleIfndef(
    struct PreprocessorState *state,
    const char *restOfLine)
{
    return handleIfdefReal(state, restOfLine, nktrue);
}

// MEMSAFE
nkbool handleElse(
    struct PreprocessorState *state,
    const char *restOfLine)
{
    return preprocessorStateFlipIfResult(state);
}

// MEMSAFE
nkbool handleEndif(
    struct PreprocessorState *state,
    const char *restOfLine)
{
    return preprocessorStatePopIfResult(state);
}

// MEMSAFE
nkbool handleUndef(
    struct PreprocessorState *state,
    const char *restOfLine)
{
    nkbool ret = nktrue;
    struct PreprocessorState *directiveParseState = NULL;
    struct PreprocessorToken *identifierToken = NULL;

    directiveParseState =
        createPreprocessorState(state->errorState, state->memoryCallbacks);
    if(!directiveParseState) {
        return nkfalse;
    }

    directiveParseState->str = restOfLine;

    // Get identifier.
    identifierToken = getNextToken(directiveParseState, nkfalse);
    if(!identifierToken) {
        destroyPreprocessorState(directiveParseState);
        return nkfalse;
    }

    if(identifierToken->type == NK_PPTOKEN_IDENTIFIER) {

        if(preprocessorStateDeleteMacro(state, identifierToken->str)) {

            // Success!

        } else {

            preprocessorStateAddError(state, "Cannot delete macro.");
            ret = nkfalse;

        }

    } else {

        preprocessorStateAddError(state, "Invalid identifier.");
        ret = nkfalse;

    }

    destroyToken(state, identifierToken);
    destroyPreprocessorState(directiveParseState);
    return ret;
}

// MEMSAFE
nkbool handleDefine(
    struct PreprocessorState *state,
    const char *restOfLine)
{
    nkbool ret = nktrue;
    struct PreprocessorState *directiveParseState = NULL;
    struct PreprocessorMacro *macro = NULL;
    struct PreprocessorToken *identifierToken = NULL;
    char *definition = NULL;
    nkbool expectingComma = nkfalse;
    struct PreprocessorToken *openParenToken = NULL;
    struct PreprocessorToken *argToken = NULL;

    // Setup pp state.
    directiveParseState = createPreprocessorState(
        state->errorState, state->memoryCallbacks);
    if(!directiveParseState) {
        ret = nkfalse;
        goto handleDefine_cleanup;
    }
    directiveParseState->str = restOfLine;

    // Setup new macro.
    macro = createPreprocessorMacro(state);
    if(!macro) {
        ret = nkfalse;
        goto handleDefine_cleanup;
    }

    // Get identifier.
    identifierToken =
        getNextToken(directiveParseState, nkfalse);
    if(!identifierToken) {
        ret = nkfalse;
        goto handleDefine_cleanup;
    }

    // That better be an identifier.
    if(identifierToken->type != NK_PPTOKEN_IDENTIFIER) {
        preprocessorStateAddError(state, "Expected identifier after #define.");
        ret = nkfalse;
        goto handleDefine_cleanup;
    }

    if(!preprocessorMacroSetIdentifier(
            state, macro, identifierToken->str))
    {
        ret = nkfalse;
        goto handleDefine_cleanup;
    }

    if(!skipWhitespaceAndComments(
            directiveParseState, nkfalse, nkfalse))
    {
        ret = nkfalse;
        goto handleDefine_cleanup;
    }

    // Check for arguments (next char == '(').
    if(directiveParseState->str[directiveParseState->index] == '(') {

        // Skip the open paren.
        openParenToken =
            getNextToken(directiveParseState, nkfalse);
        if(!openParenToken) {
            ret = nkfalse;
            goto handleDefine_cleanup;
        }

        destroyToken(state, openParenToken);
        openParenToken = NULL;

        // Parse the argument list.

        argToken = getNextToken(directiveParseState, nkfalse);

        macro->functionStyleMacro = nktrue;

        if(!argToken) {

            // We're done, but with an error!
            preprocessorStateAddError(
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
                        if(!preprocessorMacroAddArgument(
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
                argToken = getNextToken(directiveParseState, nkfalse);

                // We're not supposed to hit the end of the
                // list here.
                if(!argToken) {
                    preprocessorStateAddError(state, "Incomplete argument list in #define.");
                    ret = nkfalse;
                    break;
                }
            }
        }

        // Skip up to the actual definition.
        if(!skipWhitespaceAndComments(directiveParseState, nkfalse, nkfalse)) {
            ret = nkfalse;
            goto handleDefine_cleanup;
        }
    }

    // Read the macro definition.
    definition = nkppStripCommentsAndTrim(
        state,
        directiveParseState->str + directiveParseState->index);
    if(!definition) {
        ret = nkfalse;
        goto handleDefine_cleanup;
    }

    // Set it.
    if(!preprocessorMacroSetDefinition(
            state, macro, definition))
    {
        ret = nkfalse;
        goto handleDefine_cleanup;
    }

    // Disallow multiple definitions.
    if(preprocessorStateFindMacro(state, macro->identifier)) {
        preprocessorStateAddError(state, "Multiple definitions of the same macro.");
        ret = nkfalse;
    }

    // Add definition to list, or clean up if we had an error.
    if(ret) {
        preprocessorStateAddMacro(state, macro);
        macro = NULL;
    } else {
        destroyPreprocessorMacro(state, macro);
        macro = NULL;
    }

handleDefine_cleanup:

    // Cleanup.
    if(directiveParseState) {
        destroyPreprocessorState(directiveParseState);
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
        destroyPreprocessorMacro(state, macro);
    }

    return ret;
}
