#include <stdio.h>

#include "ppcommon.h"
#include "ppmacro.h"
#include "ppstate.h"

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

char *readIdentifier(struct PreprocessorState *state)
{
    const char *str = state->str;
    nkuint32_t *i = &state->index;

    nkuint32_t start = *i;
    nkuint32_t end;
    nkuint32_t len;
    char *ret;

    while(nkiCompilerIsValidIdentifierCharacter(str[*i], *i == start)) {
        (*i)++;
    }

    end = *i;

    len = end - start;

    // TODO: Check overflow.
    ret = mallocWrapper(len + 1);

    memcpyWrapper(ret, str + start, len);
    ret[len] = 0;

    return ret;
}

char *readQuotedString(struct PreprocessorState *state)
{
    const char *str = state->str;
    nkuint32_t *i = &state->index;
    nkuint32_t start = *i;
    nkuint32_t end;
    nkuint32_t len;
    char *ret;
    nkbool backslashed = nkfalse;

    // Should only be called on the starting quote.
    assert(str[*i] == '"');

    // Skip initial quote.
    (*i)++;

    while(str[*i]) {

        if(str[*i] == '\\') {

            backslashed = !backslashed;

        } else if(!backslashed && str[*i] == '"') {

            // Skip final quote.
            (*i)++;
            break;

        } else {

            backslashed = nkfalse;

        }

        (*i)++;
    }

    end = *i;

    len = end - start;

    // TODO: Check overflow.
    ret = mallocWrapper(len + 1);

    memcpyWrapper(ret, str + start, len);
    ret[len] = 0;

    return ret;
}

char *readInteger(struct PreprocessorState *state)
{
    const char *str = state->str;
    nkuint32_t *i = &state->index;

    nkuint32_t start = *i;
    nkuint32_t end;
    nkuint32_t len;
    char *ret;

    while(nkiCompilerIsNumber(str[*i])) {
        (*i)++;
    }

    end = *i;

    len = end - start;

    // TODO: Check overflow.
    ret = mallocWrapper(len + 1);

    memcpyWrapper(ret, str + start, len);
    ret[len] = 0;

    return ret;
}

enum PreprocessorTokenType
{
    NK_PPTOKEN_IDENTIFIER,
    NK_PPTOKEN_QUOTEDSTRING, // str is string with escape characters unconverted and quotes intact!
    NK_PPTOKEN_HASH, // '#'
    NK_PPTOKEN_COMMA,
    NK_PPTOKEN_NUMBER,
    NK_PPTOKEN_OPENPAREN,
    NK_PPTOKEN_CLOSEPAREN,

    // It's the preprocessor, so we have to let anything we don't
    // understand slide through.
    NK_PPTOKEN_UNKNOWN
};

struct PreprocessorToken
{
    char *str;
    nkuint32_t type;
    nkuint32_t lineNumber;
};

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

char *preprocess(struct PreprocessorState *state, const char *str)
{
    state->str = str;

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

            if(token->type == NK_PPTOKEN_HASH) {

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

                        // Something we don't recognize? Better just
                        // output it with the hash and let the next
                        // preprocessor deal with it (GLSL, etc).
                        appendString(state, token->str);
                        appendString(state, directiveNameToken->str);

                    }

                    destroyToken(directiveNameToken);

                } else {

                    // Some non-character identifier came after the
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

                    // TODO: Check argument count.
                    // TODO: Read arguments. Allow 0 in parens?

                    struct PreprocessorState *clonedState = preprocessorStateClone(state);

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

                    // FIXME: Remove this.
                    appendString(state, ">>>");

                    appendString(state, macro->definition);

                    // FIXME: Remove this.
                    appendString(state, "<<<");

                    destroyPreprocessorState(clonedState);

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

    {
        char *ret = state->output;
        state->output = NULL;
        destroyPreprocessorState(state);
        return ret;
    }
}





int main(int argc, char *argv[])
{
    struct PreprocessorState *state = createPreprocessorState();

    printf("----------------------------------------------------------------------\n");
    printf("  Input string\n");
    printf("----------------------------------------------------------------------\n");
    printf("%s\n", testStr);

    {
        char *preprocessed = preprocess(state, testStr);

        printf("----------------------------------------------------------------------\n");
        printf("  Preprocessor output\n");
        printf("----------------------------------------------------------------------\n");
        printf("%s\n", preprocessed);

        freeWrapper(preprocessed);
    }

    return 0;
}
