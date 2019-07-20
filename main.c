#include <stdio.h>

#include "ppcommon.h"
#include "ppmacro.h"
#include "ppstate.h"
#include "pptoken.h"

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

// #define malloc(x) ASDVDFKMggkmsglkbmdflksvm
// #define free(x) kscmkcmsadlckmsdacas
// #define memcpy(x, y, z) kmsdkmsdvkldmfvklds

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

    // TODO: Numbers, quoted strings, parens, commas.

    return ret;
}

void destroyToken(struct PreprocessorToken *token)
{
    freeWrapper(token->str);
    freeWrapper(token);
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

            if(lastCharWasBackslash) {
                // This is an escaped newline.
                state->lineNumber++;
                if(actualLineCount) {
                    (*actualLineCount)++;
                }
            } else {
                break;
            }

        } else {

            lastCharWasBackslash = nkfalse;
        }

        state->index++;
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

char *deleteBackslashNewlines(const char *str)
{
    // FIXME: Check overflow.
    nkuint32_t inputLen = strlenWrapper(str);
    char *outputStr = mallocWrapper(inputLen + 1);
    nkuint32_t i;
    nkuint32_t n;

    n = 0;
    for(i = 0; i < inputLen; i++) {

        // Skip backslashes before a newline.
        if(str[i] == '\\' && str[i+1] == '\n') {
            i++;
        }

        outputStr[n++] = str[i];
    }

    outputStr[n] = 0;

    return outputStr;
}

nkbool handleDirective(
    struct PreprocessorState *state,
    const char *directive,
    const char *restOfLine)
{
    nkbool ret = nkfalse;
    struct PreprocessorState *directiveParseState = createPreprocessorState();
    char *deletedBackslashes = deleteBackslashNewlines(restOfLine);

    directiveParseState->str = deletedBackslashes;

    if(!strcmpWrapper(directive, "undef")) {

        ret = nktrue;

        // Get identifier.
        struct PreprocessorToken *identifierToken =
            getNextToken(directiveParseState, nkfalse);

        if(identifierToken->type == NK_PPTOKEN_IDENTIFIER) {

            if(preprocessorStateDeleteMacro(state, identifierToken->str)) {

                // Success!

            } else {

                // TODO: Error.
                ret = nkfalse;

            }

        } else {

            // TODO: Error.
            ret = nkfalse;

        }

        destroyToken(identifierToken);

    } else if(!strcmpWrapper(directive, "define")) {

        struct PreprocessorMacro *macro = createPreprocessorMacro();

        // Get identifier.
        struct PreprocessorToken *identifierToken =
            getNextToken(directiveParseState, nkfalse);

        char *definition = NULL;

        if(identifierToken->type == NK_PPTOKEN_IDENTIFIER) {

            // Assume success. Set to false if something fails.
            ret = nktrue;

            // FIXME: TODO: Check to see if this identifier exists as
            //   a macro first.
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
                                printf("Argument name: %s\n", token->str);

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
                            ret = nkfalse;
                            break;
                        }
                    }
                }

                // Skip up to the actual definition.
                skipWhitespaceAndComments(directiveParseState, nkfalse, nkfalse);
            }

            // Read the macro definition.
            definition = strdupWrapper(
                directiveParseState->str + directiveParseState->index);
            preprocessorMacroSetDefinition(
                macro, definition);

            printf("Define: \"%s\" = \"%s\"\n", identifierToken->str, definition);

            freeWrapper(definition);
            destroyToken(identifierToken);

        } else {

            ret = nkfalse;
        }

        // TODO: Add definition to list, or clean up if we had an
        // error.
        if(ret) {
            preprocessorStateAddMacro(state, macro);
        } else {
            destroyPreprocessorMacro(macro);
        }

    } else {

        // Unknown directive. Leave it alone.

        // FIXME: Error out.

        ret = nkfalse;
    }

    free(deletedBackslashes);
    destroyPreprocessorState(directiveParseState);
    return ret;
}

char *readMacroArgument(struct PreprocessorState *state)
{
    // Create a pristine state to read the arguments with.
    struct PreprocessorState *readerState = createPreprocessorState();
    nkuint32_t parenLevel = 0;

    // Copy input and position.
    readerState->index = state->index;
    readerState->str = state->str;

    // Skip whitespace up to the first token, but don't append
    // whitespace on this side of it.
    skipWhitespaceAndComments(readerState, nkfalse, nkfalse);

    struct PreprocessorToken *token = NULL;
    do {
        skipWhitespaceAndComments(readerState, nktrue, nkfalse);

        // Check to see if we're "done". (Zero-length arguments are
        // okay, so we have to do this at the start.)
        if(!parenLevel) {
            if(readerState->str[readerState->index] == ',' ||
                readerState->str[readerState->index] == ')')
            {
                break;
            }
        }

        // Read token and output it.
        token = getNextToken(readerState, nktrue);

        // Check for '(', ')', and ','. Do stuff with paren level.
        if(token) {
            if(token->type == NK_PPTOKEN_OPENPAREN) {
                parenLevel++;
            } else if(token->type == NK_PPTOKEN_CLOSEPAREN) {
                parenLevel--;
            }
        }

        appendString(readerState, token->str);
        destroyToken(token);

    } while(token);

    // if(token) {
    //     destroyToken(token);
    // }

    // Update read position in the source state.
    state->index = readerState->index;

    {
        char *ret = readerState->output;
        readerState->output = NULL;
        destroyPreprocessorState(readerState);
        return ret;
    }
}

void preprocessorStateClearOutput(struct PreprocessorState *state)
{
    freeWrapper(state->output);
    state->output = NULL;
}


void preprocess(struct PreprocessorState *state, const char *str)
{
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

                    // Act on anything we might understand.
                    if(!strcmpWrapper(directiveNameToken->str, "undef") ||
                        !strcmpWrapper(directiveNameToken->str, "define"))
                    {
                        nkuint32_t lineCount = 0;
                        char *line = readRestOfLine(state, &lineCount);

                        if(handleDirective(
                            state,
                            directiveNameToken->str,
                            line))
                        {

                            // Directive is valid. Output newlines to
                            // fill in the space used by the
                            // directive.
                            while(lineCount) {
                                appendChar(state, '\n');
                                lineCount--;
                            }

                        } else {

                            // Directive was invalid, or something we
                            // don't understand. Puke it back into the
                            // output.
                            appendString(state, token->str);
                            appendString(state, directiveNameToken->str);
                            appendString(state, line);

                        }

                        // Clean up.
                        freeWrapper(line);

                    } else {

                        // TODO: Stringification?

                        // Something we don't recognize? Better just
                        // output it with the hash and let the next
                        // preprocessor deal with it (GLSL, etc).
                        appendString(state, token->str);
                        appendString(state, directiveNameToken->str);

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

                    struct PreprocessorState *clonedState = preprocessorStateClone(state);
                    nkuint32_t startLineNumber = state->lineNumber;

                    // Input is the macro definition. Output is
                    // appending to the "parent" state.

                    // clonedState->str = macro->definition; // FIXME: Redundant?

                    if(macro->arguments) {

                        skipWhitespaceAndComments(state, nkfalse, nkfalse);

                        if(state->str[state->index] == '(') {

                            // Skip open paren.
                            skipChar(state, nkfalse);


                            {
                                struct PreprocessorMacroArgument *argument = macro->arguments;

                                while(argument) {

                                    // Read the macro argument.
                                    char *argumentText = readMacroArgument(state);

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
                                            // TODO: Error out.
                                            break;
                                        }

                                    } else {

                                        // Expect ')'
                                        if(state->str[state->index] == ')') {
                                            skipChar(state, nkfalse);
                                        } else {
                                            // TODO: Error out.
                                            break;
                                        }
                                    }

                                    argument = argument->next;

                                }

                            }

                        } else {

                            // FIXME: Error out. Expected argument list.

                        }

                    } else {
                        // No macros. Nothing to set up on cloned
                        // state.
                    }

                    // Preprocess the macro into place.
                    {
                        // Clear input
                        preprocessorStateClearOutput(clonedState);

                        preprocess(clonedState, macro->definition);

                        // // FIXME: Remove this.
                        // appendString(state, ">>>");

                        appendString(state, clonedState->output);

                        // // FIXME: Remove this.
                        // appendString(state, "<<<");

                        // Clean up.
                        destroyPreprocessorState(clonedState);
                    }

                    // TODO: Emit file and line directives.


                    // TODO: ...
                    //   Read arguments.
                    //     For each argument...
                    //       Read tokens until we get more ')' than the number of '(' that we've passed or...
                    //         we hit a ','.
                    //       Use the preprocessor system itself to output these into a buffer (for each arg) as we go.
                    //         That will also skip quoted strings correctly.
                    //         But also maybe that'll parse pp directives inside of an argument. Do we want that?
                    //         Should we use a cloned or fresh state for that?
                    //           Cloned macros will show up later in the definition output.
                    //           Use fresh state here to avoid double-processing.
                    //   Set them up as macros in the cloned state.
                    //   Effectively #define argumentName valueGiven.
                    //   Preprocess macro->definition with the cloned state, with output targeted at original buffer.
                    //     That won't change the line number in the *parent* (cloned-from) state.
                    //     It will change the number of lines in the actual output, though, so emit #file and #line directives if clonedState->lineNumber > 1.
                    //     That will also activate directives in the definition, but they won't alter the parent state do we want that?



                } else {

                    appendString(state, token->str);

                }

            } else {

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
