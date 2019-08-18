#include "ppcommon.h"

struct NkppDirectiveMapping
{
    const char *identifier;
    nkbool (*handler)(struct NkppState *, const char*);
};

// TODO: Add these...
//   include
//   if (with more complicated expressions)
//   elif (implement "if" first)
//   warning (passthrough?)
//   pragma? (passthrough?)
//   ... anything else I think of
struct NkppDirectiveMapping nkppDirectiveMapping[] = {
    { "undef",   nkppDirective_undef   },
    { "define",  nkppDirective_define  },
    { "ifdef",   nkppDirective_ifdef   },
    { "ifndef",  nkppDirective_ifndef  },
    { "else",    nkppDirective_else    },
    { "endif",   nkppDirective_endif   },
    { "line",    nkppDirective_line    },
    { "error",   nkppDirective_error   },
    { "include", nkppDirective_include },
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

        // Now that we've checked the syntax, we can skip doing the
        // actual work if we're inside a failed #if block.
        if(!state->nestedFailedIfs) {

            if(nkppStateDeleteMacro(state, identifierToken->str)) {

                // Success!

            } else {

                nkppStateAddError(state, "Cannot delete macro.");
                ret = nkfalse;

            }

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

        // Now that we've checked the syntax, we can skip doing the
        // actual work if we're inside a failed #if block.
        if(!state->nestedFailedIfs) {

            // Add the macro.
            nkppStateAddMacro(state, macro);
            macro = NULL;

        }

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
    struct NkppState *childState = NULL;
    struct NkppState *parserState = NULL;
    nkuint32_t num = 0;
    char *trimmedLine = NULL;
    nkbool ret = nktrue;
    struct NkppToken *lineNumberToken = NULL;
    struct NkppToken *filenameToken = NULL;

    char *newFilename = NULL;
    nkuint32_t newLineNumber = state->lineNumber;

    childState = nkppStateClone(state, nkfalse);
    if(!childState) {
        ret = nkfalse;
        goto nkppDirective_line_cleanup;
    }

    // Preprocess this line first.
    if(!nkppStateExecute(childState, restOfLine)) {
        ret = nkfalse;
        goto nkppDirective_line_cleanup;
    }

    // Trim the line down.
    trimmedLine = nkppStripCommentsAndTrim(state, childState->output);
    if(!trimmedLine) {
        ret = nkfalse;
        goto nkppDirective_line_cleanup;
    }

    // Create a parser state just for the tokenization functionality.
    parserState = nkppStateCreate(state->errorState, state->memoryCallbacks);
    if(!parserState) {
        ret = nkfalse;
        goto nkppDirective_line_cleanup;
    }
    parserState->str = trimmedLine;

    // Read line number.
    lineNumberToken = nkppStateInputGetNextToken(parserState, nkfalse);
    if(!lineNumberToken || lineNumberToken->type != NK_PPTOKEN_NUMBER) {
        nkppStateAddError(state, "Expected number after #line directive.");
        ret = nkfalse;
        goto nkppDirective_line_cleanup;
    }

    // Read filename.
    filenameToken = nkppStateInputGetNextToken(parserState, nkfalse);
    if(filenameToken) {
        if(filenameToken->type != NK_PPTOKEN_QUOTEDSTRING) {
            nkppStateAddError(state, "Expected quoted string for filename in #line directive.");
            ret = nkfalse;
            goto nkppDirective_line_cleanup;
        }
    }

    // Handle line number.
    ret = ret && nkppStrtol(lineNumberToken->str, &num);
    if(ret) {

        // Only actually do something if we're outside of a failed
        // "if" block.
        if(!state->nestedFailedIfs) {
            state->lineNumber = num;
        }

    } else {
        nkppStateAddError(state, "Error parsing line number.");
    }

    // Handle filename.
    if(filenameToken) {

        // Note: We don't un-escape this string, because the C
        // compiler doesn't.
        newFilename =
            nkppRemoveQuotes(state, filenameToken->str, nkfalse);

        if(!newFilename) {
            nkppStateAddError(state, "Error parsing line filename.");
            ret = nkfalse;
            goto nkppDirective_line_cleanup;
        }

    }

    // Set everything.
    state->lineNumber = newLineNumber;
    if(newFilename) {
        nkppStateSetFilename(state, newFilename);
    }

nkppDirective_line_cleanup:
    if(trimmedLine) {
        nkppFree(state, trimmedLine);
    }
    if(childState) {
        nkppStateDestroy(childState);
    }
    if(parserState) {
        nkppStateDestroy(parserState);
    }
    if(lineNumberToken) {
        nkppTokenDestroy(state, lineNumberToken);
    }
    if(filenameToken) {
        nkppTokenDestroy(state, filenameToken);
    }
    if(newFilename) {
        nkppFree(state, newFilename);
    }
    return ret;
}

nkbool nkppDirective_error(
    struct NkppState *state,
    const char *restOfLine)
{
    char *trimmedLine = nkppStripCommentsAndTrim(state, restOfLine);
    if(!trimmedLine) {
        return nkfalse;
    }

    if(!state->nestedFailedIfs) {
        nkppStateAddError(state, trimmedLine);
    }

    nkppFree(state, trimmedLine);

    // This one's a little weird. We want to resume normal behavior
    // despite having had an error.
    return nktrue;
}

nkbool nkppDirective_include_handleInclusion(
    struct NkppState *state,
    const char *unquotedName)
{
    nkbool ret = nktrue;
    char *fileData = NULL;
    NkppLoadFileCallback loadFileCallback =
        state->memoryCallbacks && state->memoryCallbacks->loadFileCallback ?
        state->memoryCallbacks->loadFileCallback : nkppDefaultLoadFileCallback;

    // Save original place.
    char *originalFilename = nkppStrdup(state, state->filename);
    nkuint32_t originalLine = state->lineNumber;
    nkuint32_t originalFailedIfs = state->nestedFailedIfs; // Always 0 here?
    nkuint32_t originalPassedIfs = state->nestedPassedIfs;
    nkuint32_t originalRecursionLevel = state->recursionLevel;
    nkuint32_t originalIndex = state->index;
    const char *originalSource = state->str;
    if(!originalFilename) {
        // Bail out before we have anything we need to clean up,
        // because clean up here is going to be a little weird.
        return nkfalse;
    }

    // (Attempt to) load the file.
    fileData =
        loadFileCallback(
            state,
            state->memoryCallbacks ? state->memoryCallbacks->userData : NULL,
            unquotedName);
    if(!fileData) {
        nkppStateAddError(state, "Could not load file to include.");
        ret = nkfalse;
        goto nkppDirective_include_handleInclusion_cleanup;
    }

    // Set up new state.
    state->index = 0;
    state->str = fileData;
    state->nestedFailedIfs = 0;
    state->nestedPassedIfs = 0;
    state->recursionLevel++;
    state->lineNumber = 0;
    if(!nkppStateSetFilename(state, unquotedName)) {
        ret = nkfalse;
        goto nkppDirective_include_handleInclusion_cleanup;
    }

    // Execute it.
    nkppStateExecute(state, fileData);

nkppDirective_include_handleInclusion_cleanup:

    if(fileData) {
        nkppFree(state, fileData);
    }

    // Restore old file name manually to avoid an extra allocation (we
    // don't want allocations during cleanup).
    nkppFree(state, state->filename);
    state->filename = originalFilename;

    // Restore all the other random things we saved on the state.
    state->lineNumber = originalLine;
    state->nestedFailedIfs = originalFailedIfs;
    state->nestedPassedIfs = originalPassedIfs;
    state->recursionLevel = originalRecursionLevel;
    state->index = originalIndex;
    state->str = originalSource;

    return ret;
}

nkbool nkppDirective_include(
    struct NkppState *state,
    const char *restOfLine)
{
    nkbool ret = nktrue;
    char *trimmedInput = NULL;
    nkuint32_t inputLen;
    char *unquotedName = NULL;
    nkuint32_t filenameEnd = 0;
    char endChar = 0;
    char *currentDirname = NULL;
    char *appendedPath = NULL;

    // Trim input.
    trimmedInput = nkppStripCommentsAndTrim(state, restOfLine);
    if(!trimmedInput) {
        ret = nkfalse;
        goto nkppDirective_include_cleanup;
    }

    // Parameter must be long enough to contain the "" or <>.
    inputLen = nkppStrlen(trimmedInput);
    if(inputLen < 2) {
        nkppStateAddError(state, "Invalid filename for #include.");
        ret = nkfalse;
        goto nkppDirective_include_cleanup;
    }

    if(trimmedInput[0] == '"') {
        endChar = '"';
    } else if(trimmedInput[0] == '<') {
        endChar = '>';
    } else {
        nkppStateAddError(state, "Expected \"\" or <> around filename in #include.");
        ret = nkfalse;
        goto nkppDirective_include_cleanup;
    }

    // Try to find the next quote.
    filenameEnd = 1;
    while(trimmedInput[filenameEnd]) {
        if(trimmedInput[filenameEnd] == endChar) {
            break;
        }
        if(trimmedInput[filenameEnd] == '\n') {
            break;
        }
        filenameEnd++;
    }
    if(trimmedInput[filenameEnd] != endChar) {
        nkppStateAddError(state, "No end quote or bracket found for #include directive.");
        ret = nkfalse;
        goto nkppDirective_include_cleanup;
    }

    // Make sure there's nothing after the end.
    if(filenameEnd != inputLen - 1) {
        nkppStateAddError(state, "Extra tokens after filename in #include directive.");
        ret = nkfalse;
        goto nkppDirective_include_cleanup;
    }

    // Make an un-quoted version of the name. Note: No un-escaping
    // happens here.
    unquotedName = nkppStrdup(state, trimmedInput + 1);
    unquotedName[filenameEnd - 1] = 0;

    // Handle path relativity.
    currentDirname = nkppPathDirname(state, state->filename);
    if(!currentDirname) {
        ret = nkfalse;
        goto nkppDirective_include_cleanup;
    }
    appendedPath = nkppPathAppend(state, currentDirname, unquotedName);
    if(!appendedPath) {
        ret = nkfalse;
        goto nkppDirective_include_cleanup;
    }

    if(!state->nestedFailedIfs) {
        if(!nkppDirective_include_handleInclusion(state, appendedPath)) {
            ret = nkfalse;
            goto nkppDirective_include_cleanup;
        }
    }

nkppDirective_include_cleanup:

    if(currentDirname) {
        nkppFree(state, currentDirname);
    }
    if(appendedPath) {
        nkppFree(state, appendedPath);
    }
    if(trimmedInput) {
        nkppFree(state, trimmedInput);
    }
    if(unquotedName) {
        nkppFree(state, unquotedName);
    }

    return ret;
}


