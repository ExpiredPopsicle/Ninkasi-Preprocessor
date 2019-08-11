#include <stdio.h>

#include "ppcommon.h"
#include "ppmacro.h"
#include "ppstate.h"
#include "pptoken.h"
#include "ppstring.h"
#include "ppdirect.h"

#include <assert.h>
#include <stdlib.h>

// ----------------------------------------------------------------------

char *testStr =
    "#define foo bar\n"
    "foo\n"
    "#undef foo\n"
    "#define foo(x) (x+x)\n"
    "foo(12345)\n"
    "\"string test\"\n"
    "\"quoted \\\"string\\\" test\"\n"
    "#undef foo\n"
    "#define multiarg(x, y, z, butts) \\\n  (x + x - y + (butts))\n"
    "#define multiarg(x, y, z, butts  \\\n  (x + x - y + (butts))\n"
    "#define multiarg(x, y, z,        \\\n  (x + x - y + (butts))\n"
    "#define multiarg(x, y, z         \\\n  (x + x - y + (butts))\n"
    "#define multiarg(x, y,           \\\n  (x + x - y + (butts))\n"
    "#include \"test.txt\"\n"
    "#include \"test.txt\"\n"
    "#define stringTest \"some \\\"quoted\\\" string\"\n"
    "stringTest\n"
    "// comment\n"
    "#define \\\n  multiline \\\n"
    "    definition \\\n"
    "    thingy\n"
    "// comment after multiline\n";

// TODO: Move this into ppstate.c
// MEMSAFE
struct NkppToken *nkppStateInputGetNextToken(
    struct NkppState *state,
    nkbool outputWhitespace)
{
    nkbool success = nktrue;
    struct NkppToken *ret = nkppMalloc(
        state, sizeof(struct NkppToken));
    if(!ret) {
        return NULL;
    }

    ret->str = NULL;
    ret->type = NK_PPTOKEN_UNKNOWN;

    if(!nkppStateInputSkipWhitespaceAndComments(state, outputWhitespace, nkfalse)) {
        nkppFree(state, ret);
        return NULL;
    }

    ret->lineNumber = state->lineNumber;

    // Bail out if we're at the end.
    if(!state->str[state->index]) {
        nkppFree(state, ret);
        return NULL;
    }

    if(nkiCompilerIsValidIdentifierCharacter(state->str[state->index], nktrue)) {

        // Read identifiers (and directives).

        ret->str = nkppStateInputReadIdentifier(state);
        ret->type = NK_PPTOKEN_IDENTIFIER;

    } else if(nkiCompilerIsNumber(state->str[state->index])) {

        // Read number.

        ret->str = nkppStateInputReadInteger(state);
        ret->type = NK_PPTOKEN_NUMBER;

    } else if(state->str[state->index] == '"') {

        // Read quoted string.

        ret->str = nkppStateInputReadQuotedString(state);
        ret->type = NK_PPTOKEN_QUOTEDSTRING;

    } else if(state->str[state->index] == '#' &&
        state->str[state->index+1] == '#')
    {
        // Double-hash (concatenation) symbol.

        ret->str = nkppMalloc(state, 3);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = state->str[state->index+1];
            ret->str[2] = 0;
        }
        ret->type = NK_PPTOKEN_DOUBLEHASH;

        success = success && nkppStateInputSkipChar(state, nkfalse);
        success = success && nkppStateInputSkipChar(state, nkfalse);

    } else if(state->str[state->index] == '#') {

        // Hash symbol.

        ret->str = nkppMalloc(state, 2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }
        ret->type = NK_PPTOKEN_HASH;

        success = success && nkppStateInputSkipChar(state, nkfalse);

    } else if(state->str[state->index] == ',') {

        // Comma.

        ret->str = nkppMalloc(state, 2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }
        ret->type = NK_PPTOKEN_COMMA;

        success = success && nkppStateInputSkipChar(state, nkfalse);

    } else if(state->str[state->index] == '(') {

        // Open parenthesis.

        ret->str = nkppMalloc(state, 2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }
        ret->type = NK_PPTOKEN_OPENPAREN;

        success = success && nkppStateInputSkipChar(state, nkfalse);

    } else if(state->str[state->index] == ')') {

        // Open parenthesis.

        ret->str = nkppMalloc(state, 2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }
        ret->type = NK_PPTOKEN_CLOSEPAREN;

        success = success && nkppStateInputSkipChar(state, nkfalse);

    } else {

        // Unknown.

        ret->str = nkppMalloc(state, 2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }

        success = success && nkppStateInputSkipChar(state, nkfalse);
    }

    if(!ret->str || !success) {
        nkppFree(state, ret->str);
        nkppFree(state, ret);
        ret = NULL;
    }

    return ret;
}

// MEMSAFE
char *nkppReadRestOfLine(
    struct NkppState *state,
    nkuint32_t *actualLineCount)
{
    nkbool lastCharWasBackslash = nkfalse;
    nkuint32_t lineStart = state->index;
    nkuint32_t lineEnd = lineStart;
    nkuint32_t lineLen = 0;
    nkuint32_t lineBufLen = 0;
    char *ret = NULL;
    nkbool overflow = nkfalse;

    while(state->str[state->index]) {

        if(state->str[state->index] == '/' && state->str[state->index + 1] == '*') {

            // C-style comment.

            // Skip initial comment maker.
            if(!nkppStateInputSkipChar(state, nktrue) || !nkppStateInputSkipChar(state, nktrue)) {
                return nkfalse;
            }

            while(state->str[state->index] && state->str[state->index + 1]) {
                if(state->str[state->index] == '*' && state->str[state->index + 1] == '/') {
                    if(!nkppStateInputSkipChar(state, nktrue) || !nkppStateInputSkipChar(state, nktrue)) {
                        return nkfalse;
                    }
                    break;
                }
                if(!nkppStateInputSkipChar(state, nktrue)) {
                    return nkfalse;
                }
            }

            lastCharWasBackslash = nkfalse;

        } else if(state->str[state->index] == '\\') {

            lastCharWasBackslash = !lastCharWasBackslash;

        } else if(state->str[state->index] == '\n') {

            if(actualLineCount) {
                (*actualLineCount)++;
            }

            if(lastCharWasBackslash) {

                // This is an escaped newline, so we're going to keep
                // going.
                lastCharWasBackslash = nkfalse;

            } else {

                // Skip that newline and bail out. We're done. Only
                // output if it's a newline to keep lines in sync
                // between input and output.
                if(!nkppStateInputSkipChar(state, state->str[state->index] == '\n')) {
                    return NULL;
                }
                break;

            }

        } else {

            lastCharWasBackslash = nkfalse;
        }

        // Skip this character. Only output if it's a newline to keep
        // lines in sync between input and output.
        if(!nkppStateInputSkipChar(state, state->str[state->index] == '\n')) {
            return NULL;
        }
    }

    // Save the whole line.
    lineEnd = state->index;

    if(lineEnd < lineStart) {
        return NULL;
    }
    lineLen = lineEnd - lineStart;

    // Add room for a NULL terminator.
    NK_CHECK_OVERFLOW_UINT_ADD(lineLen, 1, lineBufLen, overflow);
    if(overflow) {
        return NULL;
    }

    // Create and fill the return buffer.
    ret = nkppMalloc(state, lineBufLen);
    if(ret) {
        memcpyWrapper(ret, state->str + lineStart, lineLen);
        ret[lineLen] = 0;
    }

    return ret;
}

struct PreprocessorDirectiveMapping
{
    const char *identifier;
    nkbool (*handler)(struct NkppState *, const char*);
};

// TODO: Add these...
//   include
//   file
//   line
//   if (with more complicated expressions)
//   error
//   ... anything else I think of
struct PreprocessorDirectiveMapping directiveMapping[] = {
    { "undef",  nkppDirective_undef  },
    { "define", nkppDirective_define },
    { "ifdef",  nkppDirective_ifdef  },
    { "ifndef", nkppDirective_ifndef },
    { "else",   nkppDirective_else   },
    { "endif",  nkppDirective_endif  },
};

nkuint32_t directiveMappingLen =
    sizeof(directiveMapping) /
    sizeof(struct PreprocessorDirectiveMapping);

// MEMSAFE
nkbool directiveIsValid(
    const char *directive)
{
    nkuint32_t i;
    for(i = 0; i < directiveMappingLen; i++) {
        if(!strcmpWrapper(directive, directiveMapping[i].identifier)) {
            return nktrue;
        }
    }
    return nkfalse;
}

// MEMSAFE
nkbool handleDirective(
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
    for(i = 0; i < directiveMappingLen; i++) {
        if(!strcmpWrapper(directive, directiveMapping[i].identifier)) {
            ret = directiveMapping[i].handler(state, deletedBackslashes);
            break;
        }
    }

    // Spit out an error if we couldn't find a matching directive.
    if(i == directiveMappingLen) {
        nkppStateAddError(state, "Unknown directive.");
        ret = nkfalse;
    }

    // Update file/line markers, in case they've changed.
    nkppStateFlagFileLineMarkersForUpdate(state);

    nkppFree(state, deletedBackslashes);

    return ret;
}

nkbool preprocess(
    struct NkppState *state,
    const char *str,
    nkuint32_t recursionLevel);

// MEMSAFE (except call to preprocess())
nkbool executeMacro(
    struct NkppState *state,
    struct NkppMacro *macro,
    nkuint32_t recursionLevel)
{
    struct NkppState *clonedState;
    nkbool ret = nktrue;
    char *unstrippedArgumentText = NULL;
    char *argumentText = NULL;
    struct NkppMacro *newMacro = NULL;

    clonedState = nkppCloneState(state);
    if(!clonedState) {
        return nkfalse;
    }

    // Input is the macro definition. Output is
    // appending to the "parent" state.

    if(macro->arguments || macro->functionStyleMacro) {

        if(!nkppStateInputSkipWhitespaceAndComments(state, nkfalse, nkfalse)) {
            ret = nkfalse;
            goto executeMacro_cleanup;
        }

        if(state->str[state->index] == '(') {

            struct NkppMacroArgument *argument = macro->arguments;

            // Skip open paren.
            if(!nkppStateInputSkipChar(state, nkfalse)) {
                ret = nkfalse;
                goto executeMacro_cleanup;
            }

            while(argument) {

                // Read the macro argument.
                unstrippedArgumentText = nkppStateInputReadMacroArgument(state);
                if(!unstrippedArgumentText) {
                    ret = nkfalse;
                    goto executeMacro_cleanup;
                }

                argumentText = nkppStripCommentsAndTrim(
                    state, unstrippedArgumentText);
                if(!argumentText) {
                    ret = nkfalse;
                    goto executeMacro_cleanup;
                }

                nkppFree(state, unstrippedArgumentText);
                unstrippedArgumentText = NULL;

                // Add the argument as a macro to
                // the new cloned state.
                {
                    newMacro = createNkppMacro(state);
                    if(!newMacro) {
                        ret = nkfalse;
                        goto executeMacro_cleanup;
                    }

                    if(!preprocessorMacroSetIdentifier(state, newMacro, argument->name)) {
                        ret = nkfalse;
                        goto executeMacro_cleanup;
                    }

                    if(!preprocessorMacroSetDefinition(state, newMacro, argumentText)) {
                        ret = nkfalse;
                        goto executeMacro_cleanup;
                    }

                    nkppStateAddMacro(clonedState, newMacro);
                    newMacro = NULL;
                }

                nkppFree(state, argumentText);
                argumentText = NULL;

                if(!nkppStateInputSkipWhitespaceAndComments(state, nkfalse, nkfalse)) {
                    ret = nkfalse;
                    goto executeMacro_cleanup;
                }

                if(argument->next) {

                    // Expect ','
                    if(state->str[state->index] == ',') {
                        if(!nkppStateInputSkipChar(state, nkfalse)) {
                            ret = nkfalse;
                            goto executeMacro_cleanup;
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
                    goto executeMacro_cleanup;
                }
            } else {
                nkppStateAddError(state, "Expected ')'.");
                ret = nkfalse;
            }

        } else {
            nkppStateAddError(state, "Expected argument list.");
            ret = nkfalse;
        }

    } else {

        // No arguments. Nothing to set up on cloned
        // state.

    }

    // Preprocess the macro into place.
    if(ret) {

        // Clear output from the cloned state.
        nkppStateOutputClear(clonedState);

        // Feed the macro definition through it.
        if(!preprocess(
                clonedState,
                macro->definition ? macro->definition : "",
                recursionLevel + 1))
        {
            ret = nkfalse;
        }

        // Write output.
        if(clonedState->output) {
            if(!nkppStateOutputAppendString(state, clonedState->output)) {
                ret = nkfalse;
                goto executeMacro_cleanup;
            }
        }

        nkppStateFlagFileLineMarkersForUpdate(state);
    }

executeMacro_cleanup:

    // Clean up.
    if(clonedState) {
        nkppDestroyState(clonedState);
    }
    if(unstrippedArgumentText) {
        nkppFree(state, unstrippedArgumentText);
    }
    if(argumentText) {
        nkppFree(state, argumentText);
    }
    if(newMacro) {
        destroyNkppMacro(state, newMacro);
    }

    return ret;
}

// MEMSAFE (ish)
nkbool handleStringification(
    struct NkppState *state,
    const char *macroName,
    nkuint32_t recursionLevel)
{
    nkbool ret = nkfalse;
    char *escapedStr = NULL;
    struct NkppState *macroState = NULL;
    struct NkppMacro *macro =
        nkppStateFindMacro(
            state, macroName);

    if(macro) {

        macroState = nkppCloneState(state);
        if(macroState) {

            nkppStateOutputClear(macroState);

            if(executeMacro(macroState, macro, recursionLevel)) {

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

                nkppDestroyState(macroState);

                ret = nktrue;
            }
        }

    } else {
        nkppStateAddError(
            state,
            "Unknown input for stringification.");
    }

    return ret;
}

// MEMSAFE
nkbool preprocess(
    struct NkppState *state,
    const char *str,
    nkuint32_t recursionLevel)
{
    nkbool ret = nktrue;
    struct NkppToken *token = NULL;
    struct NkppToken *directiveNameToken = NULL;
    char *line = NULL;

    // FIXME: Maybe make this less arbitraty.
    if(recursionLevel > 20) {
        nkppStateAddError(state, "Arbitrary recursion limit reached.");
        return nkfalse;
    }

    state->str = str;
    state->index = 0;

    while(state->str && state->str[state->index]) {

        token = nkppStateInputGetNextToken(state, nktrue);

        if(token) {

            if(token->type == NK_PPTOKEN_DOUBLEHASH) {

                // Output nothing. This is the symbol concatenation
                // token, and it does its job by effectively just
                // dropping out.

            } else if(token->type == NK_PPTOKEN_HASH) {

                // Make sure there's the start of a valid identifier
                // as the very next character. We don't support
                // whitespace between the hash and a directive name.
                if(nkiCompilerIsValidIdentifierCharacter(state->str[state->index], nktrue)) {

                    // Get the directive name.
                    directiveNameToken = nkppStateInputGetNextToken(state, nktrue);

                    if(!directiveNameToken) {

                        ret = nkfalse;

                    } else {

                        // Check to see if this is something we
                        // understand by going through the *actual*
                        // list of directives.
                        if(directiveIsValid(directiveNameToken->str)) {

                            nkuint32_t lineCount = 0;

                            line = nkppReadRestOfLine(
                                state, &lineCount);

                            if(!line) {

                                ret = nkfalse;

                            } else {

                                if(handleDirective(
                                        state,
                                        directiveNameToken->str,
                                        line))
                                {

                                    // That went well.

                                } else {

                                    // I don't know if we really need
                                    // to report an error here,
                                    // because an error would have
                                    // been added in handleDirective()
                                    // for whatever went wrong.
                                    nkppStateAddError(
                                        state, "Bad directive.");
                                    ret = nkfalse;
                                }

                                // Clean up.
                                nkppFree(state, line);
                                line = NULL;

                            }

                        } else {

                            if(!handleStringification(state, directiveNameToken->str, recursionLevel)) {
                                ret = nkfalse;
                            }
                        }

                        destroyToken(state, directiveNameToken);
                        directiveNameToken = NULL;

                    }

                } else {

                    // Some non-directive identifier came after the
                    // '#', so we're going to ignore it and just
                    // output it directly as though it's not a
                    // preprocessor directive.
                    if(!nkppStateOutputAppendString(state, token->str)) {
                        ret = nkfalse;
                    }

                }


            } else if(token->type == NK_PPTOKEN_IDENTIFIER) {

                // See if we can find a macro with this name.
                struct NkppMacro *macro =
                    nkppStateFindMacro(
                        state, token->str);

                if(macro) {

                    // Execute that macro.
                    if(!executeMacro(state, macro, recursionLevel)) {
                        ret = nkfalse;
                    }

                } else {

                    // This is an identifier, but it's not a defined
                    // macro.
                    if(!nkppStateOutputAppendString(state, token->str)) {
                        ret = nkfalse;
                    }

                }

            } else {

                // We don't know what this is. It's probably not for
                // us. Just pass it through.
                if(!nkppStateOutputAppendString(state, token->str)) {
                    ret = nkfalse;
                }

            }

            destroyToken(state, token);
            token = NULL;

        } else {
            break;
        }

    }

    return ret;
}

// MEMSAFE
char *loadFile(
    struct NkppState *state,
    const char *filename)
{
    FILE *in = fopen(filename, "rb");
    nkuint32_t fileSize = 0;
    char *ret;
    int c;

    if(!in) {
        return NULL;
    }

    ret = nkppMalloc(state, fileSize + 1);
    if(!ret) {
        fclose(in);
        return NULL;
    }

    ret[0] = 0;

    while((c = fgetc(in)) != EOF) {

        char *newRet;
        ret[fileSize] = c;
        fileSize++;

        // Allocate new chunk.
        newRet = nkppRealloc(state, ret, fileSize + 1);
        if(!newRet) {
            nkppFree(state, ret);
            fclose(in);
            return NULL;
        }
        ret = newRet;

        ret[fileSize] = 0;
    }

    fclose(in);

    return ret;
}

int main(int argc, char *argv[])
{
    nkuint32_t counter;

    // 10691?
    // for(nkuint32_t counter = 4200; counter < 2000000; counter++) {
    // for(nkuint32_t counter = 0; counter < 2430; counter++) {
    // for(nkuint32_t counter = 0; counter < 1; counter++) {
    // for(nkuint32_t counter = 18830; counter < 2000000; counter++) {
    for(counter = 18830; counter < 18831; counter++) {
    // for(nkuint32_t counter = 0; counter < 18831; counter++) {
    // for(nkuint32_t counter = 2432; counter < 18831; counter++) {

        struct NkppErrorState errorState;
        struct NkppState *state;
        char *testStr2;

        // nkuint32_t allocLimit = ~(nkuint32_t)0;
        // nkuint32_t memLimit = ~(nkuint32_t)0;
        // if(argc > 2) {
        //     memLimit = atoi(argv[2]);
        // }
        // if(argc > 1) {
        //     allocLimit = atol(argv[1]);
        // }
        // setAllocationFailureTestLimits(
        //     memLimit, allocLimit);

        setAllocationFailureTestLimits(
            ~(nkuint32_t)0, counter);

        state = nkppCreateState(&errorState, NULL);
        if(!state) {
            printf("Allocation failure on state creation.\n");
            // return 0;
            continue;
        }

        testStr2 = loadFile(state, "test.txt");
        if(!testStr2) {
            printf("Allocation failure on file load.\n");
            nkppDestroyState(state);
            // return 0;
            continue;
        }

        errorState.firstError = NULL;
        errorState.lastError = NULL;

        printf("----------------------------------------------------------------------\n");
        printf("  Input string\n");
        printf("----------------------------------------------------------------------\n");
        printf("%s\n", testStr2);

        {
            printf("----------------------------------------------------------------------\n");
            printf("  Whatever\n");
            printf("----------------------------------------------------------------------\n");

            state->writePositionMarkers = nktrue;
            preprocess(state, testStr2, 0);

            printf("----------------------------------------------------------------------\n");
            printf("  Preprocessor output\n");
            printf("----------------------------------------------------------------------\n");
            if(state->output) {
                printf("%s\n", state->output);
            }

        }

        while(errorState.firstError) {

            printf("error: %s:%ld: %s\n",
                errorState.firstError->filename,
                (long)errorState.firstError->lineNumber,
                errorState.firstError->text);

            {
                struct NkppError *next = errorState.firstError->next;
                nkppFree(state, errorState.firstError->filename);
                nkppFree(state, errorState.firstError->text);
                nkppFree(state, errorState.firstError);
                errorState.firstError = next;
            }
        }

        nkppFree(state, testStr2);

        nkppDestroyState(state);

    }

    return 0;
}
