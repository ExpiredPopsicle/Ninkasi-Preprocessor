#include "ppcommon.h"

struct NkppDirectiveMapping
{
    const char *identifier;
    nkbool (*handler)(struct NkppState *, const char*);
};

// TODO: Add these...
//   include
//   file
//   line
//   if (with more complicated expressions)
//   elif
//   error
//   warning (passthrough?)
//   pragma? (passthrough?)
//   ... anything else I think of
struct NkppDirectiveMapping nkppDirectiveMapping[] = {
    { "undef",  nkppDirective_undef  },
    { "define", nkppDirective_define },
    { "ifdef",  nkppDirective_ifdef  },
    { "ifndef", nkppDirective_ifndef },
    { "else",   nkppDirective_else   },
    { "endif",  nkppDirective_endif  },
    { "line",   nkppDirective_line   },
};

static nkuint32_t nkppDirectiveMappingLen =
    sizeof(nkppDirectiveMapping) /
    sizeof(struct NkppDirectiveMapping);

nkbool nkppDirectiveIsValid(
    const char *directive)
{
    nkuint32_t i;
    for(i = 0; i < nkppDirectiveMappingLen; i++) {
        if(!nkppStrcmp(directive, nkppDirectiveMapping[i].identifier)) {
            return nktrue;
        }
    }
    return nkfalse;
}

nkbool nkppDirectiveHandleDirective(
    struct NkppState *state,
    const char *directive,
    const char *restOfLine)
{
    nkbool ret = nkfalse;
    char *deletedBackslashes;
    nkuint32_t i;

    // Reformat the block so we don't have to worry about escaped
    // newlines and stuff.
    deletedBackslashes =
        nkppDeleteBackslashNewlines(
            state,
            restOfLine);
    if(!deletedBackslashes) {
        return nkfalse;
    }

    // Figure out which handler this corresponds to and execute it.
    for(i = 0; i < nkppDirectiveMappingLen; i++) {
        if(!nkppStrcmp(directive, nkppDirectiveMapping[i].identifier)) {
            ret = nkppDirectiveMapping[i].handler(state, deletedBackslashes);
            break;
        }
    }

    // Spit out an error if we couldn't find a matching directive.
    if(i == nkppDirectiveMappingLen) {
        nkppStateAddError(state, "Unknown directive.");
        ret = nkfalse;
    }

    // Update file/line markers, in case they've changed.
    nkppStateFlagFileLineMarkersForUpdate(state);

    nkppFree(state, deletedBackslashes);

    return ret;
}

// ----------------------------------------------------------------------
// Directives

nkbool nkppDirective_ifdefReal(
    struct NkppState *state,
    const char *restOfLine,
    nkbool invert)
{
    nkbool ret = nkfalse;
    struct NkppState *directiveParseState =
        nkppStateCreate(state->errorState, state->memoryCallbacks);
    struct NkppToken *identifierToken = NULL;
    struct NkppMacro *macro = NULL;

    if(directiveParseState) {

        directiveParseState->recursionLevel = state->recursionLevel + 1;
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
        nkppTokenDestroy(state, identifierToken);
    }

    if(directiveParseState) {
        nkppStateDestroy(directiveParseState);
    }

    return ret;
}

nkbool nkppDirective_ifdef(
    struct NkppState *state,
    const char *restOfLine)
{
    return nkppDirective_ifdefReal(state, restOfLine, nkfalse);
}

nkbool nkppDirective_ifndef(
    struct NkppState *state,
    const char *restOfLine)
{
    return nkppDirective_ifdefReal(state, restOfLine, nktrue);
}

nkbool nkppDirective_else(
    struct NkppState *state,
    const char *restOfLine)
{
    return nkppStateFlipIfResult(state);
}

nkbool nkppDirective_endif(
    struct NkppState *state,
    const char *restOfLine)
{
    return nkppStatePopIfResult(state);
}

nkbool nkppDirective_undef(
    struct NkppState *state,
    const char *restOfLine)
{
    nkbool ret = nktrue;
    struct NkppState *directiveParseState = NULL;
    struct NkppToken *identifierToken = NULL;

    directiveParseState =
        nkppStateCreate(state->errorState, state->memoryCallbacks);
    if(!directiveParseState) {
        return nkfalse;
    }

    directiveParseState->str = restOfLine;
    directiveParseState->recursionLevel = state->recursionLevel + 1;

    // Get identifier.
    identifierToken = nkppStateInputGetNextToken(directiveParseState, nkfalse);
    if(!identifierToken) {
        nkppStateDestroy(directiveParseState);
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

    nkppTokenDestroy(state, identifierToken);
    nkppStateDestroy(directiveParseState);
    return ret;
}

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
    directiveParseState = nkppStateCreate(
        state->errorState, state->memoryCallbacks);
    if(!directiveParseState) {
        ret = nkfalse;
        goto nkppDirective_define_cleanup;
    }
    directiveParseState->str = restOfLine;
    directiveParseState->recursionLevel = state->recursionLevel + 1;

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

        nkppTokenDestroy(state, openParenToken);
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
            nkppTokenDestroy(state, argToken);
            argToken = NULL;

        } else {

            // Argument list with actual arguments detected.

            while(1) {

                if(expectingComma) {

                    if(argToken->type == NK_PPTOKEN_CLOSEPAREN) {

                        // We're done. Bail out.
                        nkppTokenDestroy(state, argToken);
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
                        nkppTokenDestroy(state, argToken);
                        argToken = NULL;
                        break;

                    }
                }

                // Next token.
                nkppTokenDestroy(state, argToken);
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
        nkppStateDestroy(directiveParseState);
    }
    if(identifierToken) {
        nkppTokenDestroy(state, identifierToken);
    }
    if(definition) {
        nkppFree(state, definition);
    }
    if(openParenToken) {
        nkppTokenDestroy(state, openParenToken);
    }
    if(macro) {
        nkppMacroDestroy(state, macro);
    }

    return ret;
}

nkbool nkppDirective_line(
    struct NkppState *state,
    const char *restOfLine)
{
    struct NkppState *childState;
    nkuint32_t num = 0;
    char *trimmedLine = NULL;
    nkbool ret = nktrue;

    childState = nkppStateClone(state, nkfalse);
    if(!childState) {
        return nkfalse;
    }

    if(!nkppStateExecute(childState, restOfLine)) { // FIXME: Correct recursion level.
        nkppStateDestroy(childState);
        return nkfalse;
    }

    trimmedLine = nkppStripCommentsAndTrim(state, childState->output);
    if(!trimmedLine) {
        nkppStateDestroy(childState);
        return nkfalse;
    }

    ret = nkppStrtol(trimmedLine, &num);
    if(ret) {
        state->lineNumber = num;
    } else {
        nkppStateAddError(state, "Expected number after #line directive.");
    }

    nkppFree(state, trimmedLine);
    nkppStateDestroy(childState);
    return ret;
}
