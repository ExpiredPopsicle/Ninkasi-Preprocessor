#include <stdio.h>
#include <malloc.h>
#include <string.h>

typedef unsigned int nkuint32_t;
typedef int nkint32_t;
typedef unsigned int nkbool;
typedef unsigned char nkuint8_t;
#define nktrue 1
#define nkfalse 0

// ----------------------------------------------------------------------
// Stuff copied from nktoken.c

nkbool nkiCompilerIsValidIdentifierCharacter(char c, nkbool isFirstCharacter)
{
    if(!isFirstCharacter) {
        if(c >= '0' && c <= '9') {
            return nktrue;
        }
    }

    if(c == '_') return nktrue;
    if(c >= 'a' && c <= 'z') return nktrue;
    if(c >= 'A' && c <= 'Z') return nktrue;

    return nkfalse;
}

nkbool nkiCompilerIsWhitespace(char c)
{
    if(c == ' ' || c == '\n' || c == '\t' || c == '\r') {
        return nktrue;
    }
    return nkfalse;
}

void nkiCompilerPreprocessorSkipWhitespace(
    const char *str,
    nkuint32_t *k)
{
    while(str[*k] && nkiCompilerIsWhitespace(str[*k])) {

        // Bail out when we get to the end of the line.
        if(str[*k] == '\n') {
            break;
        }

        (*k)++;
    }
}

nkbool nkiCompilerIsNumber(char c)
{
    return (c >= '0' && c <= '9');
}

void nkiMemcpy(void *dst, const void *src, nkuint32_t len)
{
    nkuint32_t i;
    for(i = 0; i < len; i++) {
        ((char*)dst)[i] = ((const char*)src)[i];
    }
}

// ----------------------------------------------------------------------

#define mallocWrapper(x) malloc(x)
#define memcpyWrapper(x, y, z) nkiMemcpy(x, y, z)
#define freeWrapper(x) free(x)
#define reallocWrapper(x, y) realloc(x, y)
#define strlenWrapper(x) strlen(x)
#define strcmpWrapper(x, y) strcmp(x, y)
#define strdupWrapper(x) strdup(x)

#define memcpy(x, y, z) kjdvkjndfvkdfmvkmdfvklm

// struct PreprocessorMacro
// {
//     char *identifier;
//     nkuint32_t argumentCount;
//     char **argumentNames;
//     char *definition;
// };

struct PreprocessorState
{
    const char *str;
    nkuint32_t index;
    nkuint32_t lineNumber;

    char *output;

    // struct PreprocessorMacro *macros;
    // nkuint32_t macroCount;
};

struct PreprocessorState *createPreprocessorState(void)
{
    struct PreprocessorState *ret =
        mallocWrapper(sizeof(struct PreprocessorState));
    ret->str = NULL;
    ret->index = 0;
    ret->lineNumber = 1;
    ret->output = NULL;
    return ret;
}

void destroyPreprocessorState(struct PreprocessorState *state)
{
    freeWrapper(state->output);
    freeWrapper(state);
}

void appendString(struct PreprocessorState *state, const char *str)
{
    nkuint32_t oldLen = state->output ? strlenWrapper(state->output) : 0;
    // TODO: Check overflow.
    nkuint32_t newLen = oldLen + strlenWrapper(str);

    // TODO: Check overflow.
    state->output = reallocWrapper(state->output, newLen + 1);

    memcpyWrapper(state->output + oldLen, str, strlenWrapper(str));
    state->output[newLen] = 0;
}

void appendChar(struct PreprocessorState *state, char c)
{
    char str[2] = { c, 0 };
    appendString(state, str);
}

void skipChar(struct PreprocessorState *state, nkbool output)
{
    if(output) {
        appendChar(state, state->str[state->index]);
    }
    state->index++;
}

void skipWhitespaceAndComments(
    struct PreprocessorState *state,
    nkbool output,
    nkbool stopAtNewline)
{
    while(state->str[state->index]) {

        // Skip comments (but output them).
        if(state->str[state->index] == '/' && state->str[state->index] == '/') {

            while(state->str[state->index] && state->str[state->index] != '\n') {
                skipChar(state, output);
            }

        } else if(!nkiCompilerIsWhitespace(state->str[state->index])) {

            // Non-whitespace, non-comment character found.
            break;

        }

        // Bail out when we get to the end of the line.
        if(state->str[state->index] == '\n') {
            if(stopAtNewline) {
                break;
            } else {
                state->lineNumber++;
            }
        }

        skipChar(state, output);
    }
}



char *testStr =
    "#define foo bar\n"
    "foo\n"
    "#undef foo\n"
    "#define foo(x) (x+x)\n"
    "foo(12345)\n"
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

    } else if(state->str[state->index] == '#') {

        // Hash symbol.

        ret->str = mallocWrapper(2);
        ret->str[0] = state->str[state->index];
        ret->str[1] = 0;
        ret->type = NK_PPTOKEN_HASH;

        skipChar(state, nkfalse);

    } else if(state->str[state->index] == ',') {

        // Hash symbol.

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

    printf("Handling directive: %s\n", directive);

    if(!strcmpWrapper(directive, "define")) {

        // Get identifier.
        struct PreprocessorToken *identifierToken =
            getNextToken(directiveParseState, nkfalse);
        char *definition = NULL;

        if(identifierToken->type == NK_PPTOKEN_IDENTIFIER) {

            // Assume success. Set to false if something fails.
            ret = nktrue;

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

            definition = strdupWrapper(
                directiveParseState->str + directiveParseState->index);

            printf("Define: \"%s\" = \"%s\"\n", identifierToken->str, definition);

            // TODO: Add definition to list.
            //   But only if ret is still nktrue (otherwise error).

            if(ret) {

                

            }

            freeWrapper(definition);
            destroyToken(identifierToken);

        } else {

            ret = nkfalse;
        }

    } else {

        // Unknown directive. Leave it alone.

        // FIXME: Clean up and error out.

        ret = nkfalse;
    }

    free(deletedBackslashes);
    destroyPreprocessorState(directiveParseState);
    return ret;
}

void tokenize(const char *str)
{
    struct PreprocessorState *state = createPreprocessorState();
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

                    // TODO: Do a thing.
                    struct PreprocessorToken *directiveNameToken = getNextToken(state, nktrue);

                    if(!strcmpWrapper(directiveNameToken->str, "define")) {

                        // Macro definition!

                        // // Read the macro name.
                        // struct PreprocessorToken *macroIdentifierToken =
                        //     getNextToken(state, nkfalse);

                        // freeWrapper(macroIdentifierToken->str);
                        // freeWrapper(macroIdentifierToken);

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

                        // FIXME: Maybe remove this, depending on
                        // where ownership goes.
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

                appendString(state, token->str);

            } else {

                appendString(state, token->str);

            }

            destroyToken(token);

        } else {
            break;
        }

    }

    printf("----------------------------------------------------------------------\n");
    printf("  Tokenizer output\n");
    printf("----------------------------------------------------------------------\n");
    printf("%s\n", state->output);

    destroyPreprocessorState(state);
}





int main(int argc, char *argv[])
{
    printf("----------------------------------------------------------------------\n");
    printf("  Input string\n");
    printf("----------------------------------------------------------------------\n");
    printf("%s\n", testStr);

    tokenize(testStr);

    return 0;
}
