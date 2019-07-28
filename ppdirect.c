#include "ppstate.h"
#include "pptoken.h"
#include "ppdirect.h"
#include "ppmacro.h"
#include "ppstring.h"

nkbool handleIfdefReal(
    struct PreprocessorState *state,
    const char *restOfLine,
    nkbool invert)
{
    nkbool ret = nktrue;
    struct PreprocessorState *directiveParseState =
        createPreprocessorState(state->errorState);
    struct PreprocessorToken *identifierToken;

    directiveParseState->str = restOfLine;

    // Get identifier.
    identifierToken =
        getNextToken(directiveParseState, nkfalse);

    if(identifierToken->type == NK_PPTOKEN_IDENTIFIER) {

        struct PreprocessorMacro *macro =
            preprocessorStateFindMacro(
                state, identifierToken->str);

        if(macro) {
            preprocessorStatePushIfResult(state, !invert);
        } else {
            preprocessorStatePushIfResult(state, invert);
        }

    } else {

        // That's not an identifier.
        preprocessorStateAddError(state, "Expected identifier after #ifdef/ifndef.");
        ret = nkfalse;
    }

    destroyToken(identifierToken);
    destroyPreprocessorState(directiveParseState);
    return ret;
}

nkbool handleIfdef(
    struct PreprocessorState *state,
    const char *restOfLine)
{
    return handleIfdefReal(state, restOfLine, nkfalse);
}

nkbool handleIfndef(
    struct PreprocessorState *state,
    const char *restOfLine)
{
    return handleIfdefReal(state, restOfLine, nktrue);
}

nkbool handleElse(
    struct PreprocessorState *state,
    const char *restOfLine)
{
    return preprocessorStateFlipIfResult(state);
}

nkbool handleEndif(
    struct PreprocessorState *state,
    const char *restOfLine)
{
    return preprocessorStatePopIfResult(state);
}

nkbool handleUndef(
    struct PreprocessorState *state,
    const char *restOfLine)
{
    nkbool ret = nktrue;
    struct PreprocessorState *directiveParseState =
        createPreprocessorState(state->errorState);
    struct PreprocessorToken *identifierToken;

    directiveParseState->str = restOfLine;

    // Get identifier.
    identifierToken = getNextToken(directiveParseState, nkfalse);

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

    destroyToken(identifierToken);

    destroyPreprocessorState(directiveParseState);

    return ret;
}

nkbool handleDefine(
    struct PreprocessorState *state,
    const char *restOfLine)
{
    nkbool ret = nktrue;
    struct PreprocessorState *directiveParseState =
        createPreprocessorState(state->errorState);
    struct PreprocessorMacro *macro = createPreprocessorMacro();
    struct PreprocessorToken *identifierToken;
    char *definition = NULL;

    directiveParseState->str = restOfLine;

    // Get identifier.
    identifierToken =
        getNextToken(directiveParseState, nkfalse);

    if(identifierToken->type == NK_PPTOKEN_IDENTIFIER) {

        preprocessorMacroSetIdentifier(
            macro,
            identifierToken->str);

        skipWhitespaceAndComments(
            directiveParseState, nkfalse, nkfalse);

        // Check for arguments (next char == '(').
        if(directiveParseState->str[directiveParseState->index] == '(') {

            nkbool expectingComma = nkfalse;

            // Skip the open paren.
            struct PreprocessorToken *openParenToken =
                getNextToken(directiveParseState, nkfalse);
            destroyToken(openParenToken);

            // Parse the argument list.

            struct PreprocessorToken *token = getNextToken(directiveParseState, nkfalse);

            if(!token) {

                // FIXME: Output error message?
                // We're done, but with an error!
                ret = nkfalse;

            } else if(token->type == NK_PPTOKEN_CLOSEPAREN) {

                // Zero-argument list. Nothing to do here.

                destroyToken(token);

            } else {

                // Argument list with actual arguments detected.

                while(1) {

                    if(expectingComma) {

                        if(token->type == NK_PPTOKEN_CLOSEPAREN) {

                            // We're done. Bail out.
                            destroyToken(token);
                            break;

                        } else if(token->type == NK_PPTOKEN_COMMA) {

                            // Okay. This is what we expect, but
                            // there's nothing useful here.
                            expectingComma = nkfalse;

                        } else {

                            // Error.
                            ret = nkfalse;
                        }

                    } else {

                        if(token->type == NK_PPTOKEN_IDENTIFIER) {

                            // Valid identifier.
                            preprocessorMacroAddArgument(macro, token->str);
                            expectingComma = nktrue;

                        } else {

                            // Not a valid identifier.
                            ret = nkfalse;
                            destroyToken(token);
                            break;

                        }
                    }

                    // Next token.
                    destroyToken(token);
                    token = getNextToken(directiveParseState, nkfalse);

                    // We're not supposed to hit the end of the
                    // list here.
                    if(!token) {
                        preprocessorStateAddError(state, "Incomplete argument list in #define.");
                        ret = nkfalse;
                        break;
                    }
                }

                // FIXME: If we didn't get any arguments, indicate
                // that, for function-style #defines.
            }

            // Skip up to the actual definition.
            skipWhitespaceAndComments(directiveParseState, nkfalse, nkfalse);
        }

        // Read the macro definition.
        definition = stripCommentsAndTrim(
            directiveParseState->str + directiveParseState->index);

        preprocessorMacroSetDefinition(
            macro, definition);

        freeWrapper(definition);

    } else {

        // That's not an identifier.
        preprocessorStateAddError(state, "Expected identifier after #define.");
        ret = nkfalse;
    }

    // Disallow multiple definitions.
    if(preprocessorStateFindMacro(state, macro->identifier)) {
        preprocessorStateAddError(state, "Multiple definitions of the same macro.");
        ret = nkfalse;
    }

    // Add definition to list, or clean up if we had an error.
    if(ret) {
        preprocessorStateAddMacro(state, macro);
    } else {
        destroyPreprocessorMacro(macro);
    }

    destroyToken(identifierToken);
    destroyPreprocessorState(directiveParseState);
    return ret;
}
