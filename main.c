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
// FIXME: Make MEMSAFE (doublecheck)
struct PreprocessorToken *getNextToken(
    struct PreprocessorState *state,
    nkbool outputWhitespace)
{
    nkbool success = nktrue;
    struct PreprocessorToken *ret = mallocWrapper(sizeof(struct PreprocessorToken));
    if(!ret) {
        return NULL;
    }

    ret->str = NULL;
    ret->type = NK_PPTOKEN_UNKNOWN;

    if(!skipWhitespaceAndComments(state, outputWhitespace, nkfalse)) {
        freeWrapper(ret);
        return NULL;
    }

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
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = state->str[state->index+1];
            ret->str[2] = 0;
        }
        ret->type = NK_PPTOKEN_DOUBLEHASH;

        success = success && skipChar(state, nkfalse);
        success = success && skipChar(state, nkfalse);

    } else if(state->str[state->index] == '#') {

        // Hash symbol.

        ret->str = mallocWrapper(2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }
        ret->type = NK_PPTOKEN_HASH;

        success = success && skipChar(state, nkfalse);

    } else if(state->str[state->index] == ',') {

        // Comma.

        ret->str = mallocWrapper(2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }
        ret->type = NK_PPTOKEN_COMMA;

        success = success && skipChar(state, nkfalse);

    } else if(state->str[state->index] == '(') {

        // Open parenthesis.

        ret->str = mallocWrapper(2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }
        ret->type = NK_PPTOKEN_OPENPAREN;

        success = success && skipChar(state, nkfalse);

    } else if(state->str[state->index] == ')') {

        // Open parenthesis.

        ret->str = mallocWrapper(2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }
        ret->type = NK_PPTOKEN_CLOSEPAREN;

        success = success && skipChar(state, nkfalse);

    } else {

        // Unknown.

        ret->str = mallocWrapper(2);
        if(ret->str) {
            ret->str[0] = state->str[state->index];
            ret->str[1] = 0;
        }

        success = success && skipChar(state, nkfalse);
    }

    if(!ret->str || !success) {
        freeWrapper(ret->str);
        freeWrapper(ret);
        ret = NULL;
    }

    return ret;
}

// FIXME: Make MEMSAFE
char *readRestOfLine(
    struct PreprocessorState *state,
    nkuint32_t *actualLineCount)
{
    nkbool lastCharWasBackslash = nkfalse;
    nkuint32_t lineStart = state->index;
    nkuint32_t lineEnd = lineStart;
    nkuint32_t lineLen = 0;
    char *ret = NULL;

    while(state->str[state->index]) {

        if(state->str[state->index] == '\\') {

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
                skipChar(state, state->str[state->index] == '\n');
                break;

            }

        } else {

            lastCharWasBackslash = nkfalse;
        }

        // Skip this character. Only output if it's a newline to keep
        // lines in sync between input and output.
        skipChar(state, state->str[state->index] == '\n');
    }

    // Save the whole line.
    lineEnd = state->index;
    // TODO: Check overflow.
    lineLen = lineEnd - lineStart;
    // TODO: Check overflow.
    ret = mallocWrapper(lineLen + 1);
    if(ret) {
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

// TODO: Add these...
//   include
//   file
//   line
//   if (with more complicated expressions)
//   ... anything else I think of
struct PreprocessorDirectiveMapping directiveMapping[] = {
    { "undef",  handleUndef  },
    { "define", handleDefine },
    { "ifdef",  handleIfdef  },
    { "ifndef", handleIfndef },
    { "else",   handleElse   },
    { "endif",  handleEndif  },
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

// FIXME: Make MEMSAFE
nkbool handleDirective(
    struct PreprocessorState *state,
    const char *directive,
    const char *restOfLine)
{
    nkbool ret = nkfalse;
    char *deletedBackslashes = deleteBackslashNewlines(restOfLine);
    nkuint32_t i;

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

nkbool preprocess(
    struct PreprocessorState *state,
    const char *str,
    nkuint32_t recursionLevel);

// FIXME: Make MEMSAFE
nkbool executeMacro(
    struct PreprocessorState *state,
    struct PreprocessorMacro *macro,
    nkuint32_t recursionLevel)
{
    struct PreprocessorState *clonedState = preprocessorStateClone(state);
    nkbool ret = nktrue;

    if(!clonedState) {
        return nkfalse;
    }

    // Input is the macro definition. Output is
    // appending to the "parent" state.

    if(macro->arguments || macro->functionStyleMacro) {

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
                    if(state->str[state->index] != ')') {
                        preprocessorStateAddError(state, "Expected ')'.");
                        ret = nkfalse;
                        break;
                    }
                }

                argument = argument->next;
            }

            // Skip final ')'.
            if(state->str[state->index] == ')') {
                skipChar(state, nkfalse);
            } else {
                preprocessorStateAddError(state, "Expected ')'.");
                ret = nkfalse;
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
        if(!preprocess(
                clonedState, macro->definition,
                recursionLevel + 1))
        {
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

// FIXME: Make MEMSAFE
nkbool preprocess(
    struct PreprocessorState *state,
    const char *str,
    nkuint32_t recursionLevel)
{
    nkbool ret = nktrue;

    // FIXME: Maybe make this less arbitraty.
    if(recursionLevel > 20) {
        preprocessorStateAddError(state, "Arbitrary recursion limit reached.");
        return nkfalse;
    }

    state->str = str;
    state->index = 0;

    while(state->str[state->index]) {

        struct PreprocessorToken *token = getNextToken(state, nktrue);

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
                    struct PreprocessorToken *directiveNameToken = getNextToken(state, nktrue);

                    if(!directiveNameToken) {

                        ret = nkfalse;

                    } else {

                        // Check to see if this is something we
                        // understand by going through the *actual*
                        // list of directives.
                        if(directiveIsValid(directiveNameToken->str)) {

                            nkuint32_t lineCount = 0;
                            char *line = readRestOfLine(state, &lineCount);

                            if(handleDirective(
                                    state,
                                    directiveNameToken->str,
                                    line))
                            {

                                // That went well.

                            } else {

                                // I don't know if we really need to
                                // report an error here, because an error
                                // would have been added in
                                // handleDirective() for whatever went
                                // wrong.
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

                                executeMacro(macroState, macro, recursionLevel);

                                {
                                    char *escapedStr = escapeString(macroState->output);

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

                    }

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

                    executeMacro(state, macro, recursionLevel);

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

    return ret;
}

// MEMSAFE
char *loadFile(const char *filename)
{
    FILE *in = fopen(filename, "rb");
    nkuint32_t fileSize = 0;
    char *ret;
    int c;

    if(!in) {
        return NULL;
    }

    ret = mallocWrapper(fileSize + 1);

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
        newRet = reallocWrapper(ret, fileSize + 1);
        if(!newRet) {
            freeWrapper(ret);
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
    for(nkuint32_t counter = 2430; counter < 2000000; counter++) {
    // for(nkuint32_t counter = 0; counter < 2430; counter++) {

        struct PreprocessorErrorState errorState;
        struct PreprocessorState *state;
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

        state = createPreprocessorState(&errorState);
        if(!state) {
            printf("Allocation failure on state creation.\n");
            // return 0;
            continue;
        }

        testStr2 = loadFile("test.txt");
        if(!testStr2) {
            printf("Allocation failure on file load.\n");
            destroyPreprocessorState(state);
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

            // freeWrapper(preprocessed);
            destroyPreprocessorState(state);
        }

        while(errorState.firstError) {

            printf("error: %s:%ld: %s\n",
                errorState.firstError->filename,
                (long)errorState.firstError->lineNumber,
                errorState.firstError->text);

            {
                struct PreprocessorError *next = errorState.firstError->next;
                freeWrapper(errorState.firstError->filename);
                freeWrapper(errorState.firstError->text);
                freeWrapper(errorState.firstError);
                errorState.firstError = next;
            }
        }

        freeWrapper(testStr2);
    }

    return 0;
}
