#include <stdio.h>

#include "ppcommon.h"
#include "ppmacro.h"
#include "ppstate.h"
#include "pptoken.h"
#include "ppstring.h"
#include "ppdirect.h"

#include <assert.h>

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
struct PreprocessorToken *getNextToken(
    struct PreprocessorState *state,
    nkbool outputWhitespace)
{
    struct PreprocessorToken *ret = mallocWrapper(sizeof(struct PreprocessorToken));
    ret->str = NULL;
    ret->type = NK_PPTOKEN_UNKNOWN;

    skipWhitespaceAndComments(state, outputWhitespace, nkfalse);

    ret->lineNumber = state->lineNumber;

    // Bail out if we're at the end.
    if(!state->str[state->index]) {
        freeWrapper(ret);
        return NULL;
    }

    if(nkiCompilerIsValidIdentifierCharacter(state->str[state->index], nktrue)) {

        // Read identifiers (and directives).

        ret->str = readIdentifier(state);
        ret->type = NK_PPTOKEN_IDENTIFIER;

    } else if(nkiCompilerIsNumber(state->str[state->index])) {

        // Read number.

        ret->str = readInteger(state);
        ret->type = NK_PPTOKEN_NUMBER;

    } else if(state->str[state->index] == '"') {

        // Read quoted string.

        ret->str = readQuotedString(state);
        ret->type = NK_PPTOKEN_QUOTEDSTRING;

    } else if(state->str[state->index] == '#' &&
        state->str[state->index+1] == '#')
    {
        // Double-hash (concatenation) symbol.

        ret->str = mallocWrapper(3);
        ret->str[0] = state->str[state->index];
        ret->str[1] = state->str[state->index+1];
        ret->str[2] = 0;
        ret->type = NK_PPTOKEN_DOUBLEHASH;

        skipChar(state, nkfalse);
        skipChar(state, nkfalse);

    } else if(state->str[state->index] == '#') {

        // Hash symbol.

        ret->str = mallocWrapper(2);
        ret->str[0] = state->str[state->index];
        ret->str[1] = 0;
        ret->type = NK_PPTOKEN_HASH;

        skipChar(state, nkfalse);

    } else if(state->str[state->index] == ',') {

        // Comma.

        ret->str = mallocWrapper(2);
        ret->str[0] = state->str[state->index];
        ret->str[1] = 0;
        ret->type = NK_PPTOKEN_COMMA;

        skipChar(state, nkfalse);

    } else if(state->str[state->index] == '(') {

        // Open parenthesis.

        ret->str = mallocWrapper(2);
        ret->str[0] = state->str[state->index];
        ret->str[1] = 0;
        ret->type = NK_PPTOKEN_OPENPAREN;

        skipChar(state, nkfalse);

    } else if(state->str[state->index] == ')') {

        // Open parenthesis.

        ret->str = mallocWrapper(2);
        ret->str[0] = state->str[state->index];
        ret->str[1] = 0;
        ret->type = NK_PPTOKEN_CLOSEPAREN;

        skipChar(state, nkfalse);

    } else {

        // Unknown.

        ret->str = mallocWrapper(2);
        ret->str[0] = state->str[state->index];
        ret->str[1] = 0;

        skipChar(state, nkfalse);
    }

    return ret;
}

char *readRestOfLine(
    struct PreprocessorState *state,
    nkuint32_t *actualLineCount)
{
    nkbool lastCharWasBackslash = nkfalse;
    nkuint32_t lineStart = state->index;
    nkuint32_t lineEnd = lineStart;
    char *ret = NULL;

    while(state->str[state->index]) {

        if(state->str[state->index] == '\\') {

            lastCharWasBackslash = !lastCharWasBackslash;

        } else if(state->str[state->index] == '\n') {

            if(actualLineCount) {
                (*actualLineCount)++;
            }

            if(lastCharWasBackslash) {
                // This is an escaped newline.
                // state->lineNumber++;
            } else {
                skipChar(state, state->str[state->index] == '\n');
                break;
            }

        } else {

            lastCharWasBackslash = nkfalse;
        }

        // state->index++;

        // if(state->str[state->index] == '\n') {
        //     appendChar(state, '>');
        // }

        skipChar(state, state->str[state->index] == '\n');

        // if(state->str[state->index-1] == '\n') {
        //     appendChar(state, '<');
        // }

        // // Add corresponding newlines to the output for these
        // // lines that we're skipping.
        // appendChar(state, '\n');
    }

    lineEnd = state->index;

    {
        // TODO: Check overflow.
        nkuint32_t lineLen = lineEnd - lineStart;

        // TODO: Check overflow.
        ret = mallocWrapper(lineLen + 1);

        memcpyWrapper(ret, state->str + lineStart, lineLen);
        ret[lineLen] = 0;
    }

    return ret;
}

struct PreprocessorDirectiveMapping
{
    const char *identifier;
    nkbool (*handler)(struct PreprocessorState *, const char*);
};

nkbool handleDirective(
    struct PreprocessorState *state,
    const char *directive,
    const char *restOfLine)
{
    nkbool ret = nkfalse;
    char *deletedBackslashes = deleteBackslashNewlines(restOfLine);
    nkuint32_t i;

    // TODO: Add these...
    //   include
    //   file
    //   line
    //   if
    //   endif
    //   ifdef
    //   ifndef
    //   ... anything else I think of

    struct PreprocessorDirectiveMapping directiveMapping[] = {
        { "undef",  handleUndef },
        { "define", handleDefine },
    };

    nkuint32_t directiveMappingLen =
        sizeof(directiveMapping) /
        sizeof(struct PreprocessorDirectiveMapping);

    for(i = 0; i < directiveMappingLen; i++) {
        if(!strcmpWrapper(directive, directiveMapping[i].identifier)) {
            ret = directiveMapping[i].handler(state, deletedBackslashes);
            break;
        }
    }

    // Spit out an error if we couldn't find a matching directive.
    if(i == directiveMappingLen) {
        preprocessorStateAddError(state, "Unknown directive.");
        ret = nkfalse;
    }

    // Update file/line markers, in case they've changed.
    preprocessorStateFlagFileLineMarkersForUpdate(state);

    freeWrapper(deletedBackslashes);

    return ret;
}

nkbool preprocess(struct PreprocessorState *state, const char *str);

nkbool executeMacro(
    struct PreprocessorState *state,
    struct PreprocessorMacro *macro)
{
    struct PreprocessorState *clonedState = preprocessorStateClone(state);
    nkbool ret = nktrue;

    // Input is the macro definition. Output is
    // appending to the "parent" state.

    if(macro->arguments) {

        skipWhitespaceAndComments(state, nkfalse, nkfalse);

        if(state->str[state->index] == '(') {

            struct PreprocessorMacroArgument *argument = macro->arguments;

            // Skip open paren.
            skipChar(state, nkfalse);

            while(argument) {

                // Read the macro argument.
                char *unstrippedArgumentText = readMacroArgument(state);
                char *argumentText = stripCommentsAndTrim(unstrippedArgumentText);

                freeWrapper(unstrippedArgumentText);

                printf("Argument (%s) text: %s\n",
                    argument->name, argumentText);

                // Add the argument as a macro to
                // the new cloned state.
                {
                    struct PreprocessorMacro *newMacro = createPreprocessorMacro();
                    preprocessorMacroSetIdentifier(newMacro, argument->name);
                    preprocessorMacroSetDefinition(newMacro, argumentText);
                    preprocessorStateAddMacro(clonedState, newMacro);
                }

                freeWrapper(argumentText);

                skipWhitespaceAndComments(state, nkfalse, nkfalse);

                if(argument->next) {

                    // Expect ','
                    if(state->str[state->index] == ',') {
                        skipChar(state, nkfalse);
                    } else {
                        preprocessorStateAddError(state, "Expected ','.");
                        ret = nkfalse;
                        break;
                    }

                } else {

                    // Expect ')'
                    if(state->str[state->index] == ')') {
                        skipChar(state, nkfalse);
                    } else {
                        preprocessorStateAddError(state, "Expected ')'.");
                        ret = nkfalse;
                        break;
                    }
                }

                argument = argument->next;
            }

        } else {
            preprocessorStateAddError(state, "Expected argument list.");
            ret = nkfalse;
        }

    } else {

        // No arguments. Nothing to set up on cloned
        // state.

    }

    // Preprocess the macro into place.
    if(ret) {

        // Clear output from the cloned state.
        preprocessorStateClearOutput(clonedState);

        // Feed the macro definition through it.
        if(!preprocess(clonedState, macro->definition)) {
            ret = nkfalse;
        }

        // Write output.
        appendString(state, clonedState->output);

        preprocessorStateFlagFileLineMarkersForUpdate(state);
    }

    // Clean up.
    destroyPreprocessorState(clonedState);

    return ret;
}

// FIXME: Add recursion counter (and error).
nkbool preprocess(struct PreprocessorState *state, const char *str)
{
    nkbool ret = nktrue;

    state->str = str;
    state->index = 0;

    printf("----------------------------------------------------------------------\n");
    printf("  Tokenizing\n");
    printf("----------------------------------------------------------------------\n");

    while(state->str[state->index]) {

        struct PreprocessorToken *token = getNextToken(state, nktrue);

        if(token) {

            printf(
                "%4d Token (%3d): '%s'\n",
                token->lineNumber,
                token->type,
                token->str);

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
                    struct PreprocessorToken *directiveNameToken = getNextToken(state, nktrue);
                    nkbool recognizedDirective = nkfalse;

                    // TODO: Check to see if this is something we
                    //   understand by going through the *actual* list
                    //   of directives.
                    if(!strcmpWrapper(directiveNameToken->str, "undef") ||
                        !strcmpWrapper(directiveNameToken->str, "define"))
                    {
                        recognizedDirective = nktrue;
                    }

                    // Act on anything we might understand.
                    if(recognizedDirective) {

                        nkuint32_t lineCount = 0;
                        char *line = readRestOfLine(state, &lineCount);

                        if(handleDirective(
                            state,
                            directiveNameToken->str,
                            line))
                        {

                        } else {
                            preprocessorStateAddError(
                                state, "Bad directive.");
                            ret = nkfalse;
                        }

                        // Clean up.
                        freeWrapper(line);

                    } else {

                        // Stringification.
                        struct PreprocessorMacro *macro =
                            preprocessorStateFindMacro(
                                state, directiveNameToken->str);

                        if(macro) {

                            struct PreprocessorState *macroState =
                                preprocessorStateClone(state);

                            preprocessorStateClearOutput(macroState);

                            executeMacro(macroState, macro);


                            {
                                // TODO: Check overflow.
                                nkuint32_t bufSize = strlen(macroState->output) * 2 + 3;
                                char *escapedStr = mallocWrapper(bufSize);
                                escapedStr[0] = 0;
                                nkiDbgAppendEscaped(bufSize, escapedStr, macroState->output);

                                appendString(state, "\"");
                                appendString(state, escapedStr);
                                appendString(state, "\"");

                                freeWrapper(escapedStr);
                            }


                            // Skip past the stuff we read in the
                            // cloned state.
                            state->index = macroState->index;

                            destroyPreprocessorState(macroState);

                        } else {

                            preprocessorStateAddError(
                                state,
                                "Unknown input for stringification.");
                            ret = nkfalse;

                        }

                    }

                    destroyToken(directiveNameToken);

                } else {

                    // Some non-directive identifier came after the
                    // '#', so we're going to ignore it and just
                    // output it directly as though it's not a
                    // preprocessor directive.
                    appendString(state, token->str);

                }


            } else if(token->type == NK_PPTOKEN_IDENTIFIER) {

                // TODO: Check for macro, then do a thing if it is,
                // otherwise just output.

                struct PreprocessorMacro *macro =
                    preprocessorStateFindMacro(
                        state, token->str);

                if(macro) {

                    executeMacro(state, macro);

                //     struct PreprocessorState *clonedState = preprocessorStateClone(state);
                //     nkuint32_t startLineNumber = state->lineNumber;

                //     // Input is the macro definition. Output is
                //     // appending to the "parent" state.

                //     if(macro->arguments) {

                //         skipWhitespaceAndComments(state, nkfalse, nkfalse);

                //         if(state->str[state->index] == '(') {

                //             // Skip open paren.
                //             skipChar(state, nkfalse);


                //             {
                //                 struct PreprocessorMacroArgument *argument = macro->arguments;

                //                 while(argument) {

                //                     // Read the macro argument.
                //                     char *unstrippedArgumentText = readMacroArgument(state);
                //                     char *argumentText = stripCommentsAndTrim(unstrippedArgumentText);

                //                     freeWrapper(unstrippedArgumentText);

                //                     printf("Argument (%s) text: %s\n",
                //                         argument->name, argumentText);

                //                     // Add the argument as a macro to
                //                     // the new cloned state.
                //                     {
                //                         struct PreprocessorMacro *newMacro = createPreprocessorMacro();
                //                         preprocessorMacroSetIdentifier(newMacro, argument->name);
                //                         preprocessorMacroSetDefinition(newMacro, argumentText);
                //                         preprocessorStateAddMacro(clonedState, newMacro);
                //                     }

                //                     freeWrapper(argumentText);

                //                     skipWhitespaceAndComments(state, nkfalse, nkfalse);

                //                     if(argument->next) {

                //                         // Expect ','
                //                         if(state->str[state->index] == ',') {
                //                             skipChar(state, nkfalse);
                //                         } else {
                //                             // TODO: Error out.
                //                             break;
                //                         }

                //                     } else {

                //                         // Expect ')'
                //                         if(state->str[state->index] == ')') {
                //                             skipChar(state, nkfalse);
                //                         } else {
                //                             // TODO: Error out.
                //                             break;
                //                         }
                //                     }

                //                     argument = argument->next;

                //                 }

                //             }

                //         } else {
                //             preprocessorStateAddError(state, "Expected argument list.");
                //         }

                //     } else {

                //         // No arguments. Nothing to set up on cloned
                //         // state.

                //     }

                //     // Preprocess the macro into place.
                //     {
                //         // Clear output from the cloned state.
                //         preprocessorStateClearOutput(clonedState);

                //         // Feed the macro definition through it.
                //         preprocess(clonedState, macro->definition);

                //         // Write output.
                //         appendString(state, clonedState->output);

                //         // Clean up.
                //         destroyPreprocessorState(clonedState);
                //     }

                //     preprocessorStateFlagFileLineMarkersForUpdate(state);

                } else {

                    // This is an identifier, but it's not a defined
                    // macro.
                    appendString(state, token->str);

                }

            } else {

                // We don't know what this is. It's probably not for
                // us. Just pass it through.
                appendString(state, token->str);

            }

            destroyToken(token);

        } else {
            break;
        }

    }

    // {
    //     char *ret = state->output;
    //     state->output = NULL;
    //     destroyPreprocessorState(state);
    //     return ret;
    // }

    return ret;
}




char *loadFile(const char *filename)
{
    FILE *in = fopen(filename, "rb");
    nkuint32_t fileSize = 0;
    char *ret = mallocWrapper(fileSize + 1);
    int c;

    ret[0] = 0;

    if(!in) {
        return NULL;
    }

    while((c = fgetc(in)) != EOF) {
        ret[fileSize] = c;
        fileSize++;
        ret = reallocWrapper(ret, fileSize + 1);
        ret[fileSize] = 0;
    }

    fclose(in);

    return ret;
}

int main(int argc, char *argv[])
{
    struct PreprocessorState *state = createPreprocessorState();
    char *testStr2 = loadFile("test.txt");

    printf("----------------------------------------------------------------------\n");
    printf("  Input string\n");
    printf("----------------------------------------------------------------------\n");
    printf("%s\n", testStr2);

    {
        state->writePositionMarkers = nktrue;
        preprocess(state, testStr2);

        printf("----------------------------------------------------------------------\n");
        printf("  Preprocessor output\n");
        printf("----------------------------------------------------------------------\n");
        printf("%s\n", state->output);

        // freeWrapper(preprocessed);
        destroyPreprocessorState(state);
    }

    freeWrapper(testStr2);

    return 0;
}
