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

// MEMSAFE
char *readRestOfLine(
    struct PreprocessorState *state,
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
                if(!skipChar(state, state->str[state->index] == '\n')) {
                    return NULL;
                }
                break;

            }

        } else {

            lastCharWasBackslash = nkfalse;
        }

        // Skip this character. Only output if it's a newline to keep
        // lines in sync between input and output.
        if(!skipChar(state, state->str[state->index] == '\n')) {
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
    ret = mallocWrapper(lineBufLen);
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

// MEMSAFE
nkbool handleDirective(
    struct PreprocessorState *state,
    const char *directive,
    const char *restOfLine)
{
    nkbool ret = nkfalse;
    char *deletedBackslashes;
    nkuint32_t i;

    // Reformat the block so we don't have to worry about escaped
    // newlines and stuff.
    deletedBackslashes = deleteBackslashNewlines(restOfLine);
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

// MEMSAFE (except call to preprocess())
nkbool executeMacro(
    struct PreprocessorState *state,
    struct PreprocessorMacro *macro,
    nkuint32_t recursionLevel)
{
    struct PreprocessorState *clonedState;
    nkbool ret = nktrue;
    char *unstrippedArgumentText = NULL;
    char *argumentText = NULL;
    struct PreprocessorMacro *newMacro = NULL;

    clonedState = preprocessorStateClone(state);
    if(!clonedState) {
        return nkfalse;
    }

    // Input is the macro definition. Output is
    // appending to the "parent" state.

    if(macro->arguments || macro->functionStyleMacro) {

        if(!skipWhitespaceAndComments(state, nkfalse, nkfalse)) {
            ret = nkfalse;
            goto executeMacro_cleanup;
        }

        if(state->str[state->index] == '(') {

            struct PreprocessorMacroArgument *argument = macro->arguments;

            // Skip open paren.
            if(!skipChar(state, nkfalse)) {
                ret = nkfalse;
                goto executeMacro_cleanup;
            }

            while(argument) {

                // Read the macro argument.
                unstrippedArgumentText = readMacroArgument(state);
                if(!unstrippedArgumentText) {
                    ret = nkfalse;
                    goto executeMacro_cleanup;
                }

                argumentText = stripCommentsAndTrim(unstrippedArgumentText);
                if(!argumentText) {
                    ret = nkfalse;
                    goto executeMacro_cleanup;
                }

                freeWrapper(unstrippedArgumentText);
                unstrippedArgumentText = NULL;

                // Add the argument as a macro to
                // the new cloned state.
                {
                    newMacro = createPreprocessorMacro();
                    if(!newMacro) {
                        ret = nkfalse;
                        goto executeMacro_cleanup;
                    }

                    if(!preprocessorMacroSetIdentifier(newMacro, argument->name)) {
                        ret = nkfalse;
                        goto executeMacro_cleanup;
                    }

                    if(!preprocessorMacroSetDefinition(newMacro, argumentText)) {
                        ret = nkfalse;
                        goto executeMacro_cleanup;
                    }

                    preprocessorStateAddMacro(clonedState, newMacro);
                    newMacro = NULL;
                }

                freeWrapper(argumentText);
                argumentText = NULL;

                if(!skipWhitespaceAndComments(state, nkfalse, nkfalse)) {
                    ret = nkfalse;
                    goto executeMacro_cleanup;
                }

                if(argument->next) {

                    // Expect ','
                    if(state->str[state->index] == ',') {
                        if(!skipChar(state, nkfalse)) {
                            ret = nkfalse;
                            goto executeMacro_cleanup;
                        }
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
                if(!skipChar(state, nkfalse)) {
                    ret = nkfalse;
                    goto executeMacro_cleanup;
                }
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
                clonedState,
                macro->definition ? macro->definition : "",
                recursionLevel + 1))
        {
            ret = nkfalse;
        }

        // Write output.
        if(clonedState->output) {
            if(!appendString(state, clonedState->output)) {
                ret = nkfalse;
                goto executeMacro_cleanup;
            }
        }

        preprocessorStateFlagFileLineMarkersForUpdate(state);
    }

executeMacro_cleanup:

    // Clean up.
    if(clonedState) {
        destroyPreprocessorState(clonedState);
    }
    if(unstrippedArgumentText) {
        freeWrapper(unstrippedArgumentText);
    }
    if(argumentText) {
        freeWrapper(argumentText);
    }
    if(newMacro) {
        destroyPreprocessorMacro(newMacro);
    }

    return ret;
}

// MEMSAFE (ish)
nkbool handleStringification(
    struct PreprocessorState *state,
    const char *macroName,
    nkuint32_t recursionLevel)
{
    nkbool ret = nkfalse;
    char *escapedStr = NULL;
    struct PreprocessorState *macroState = NULL;
    struct PreprocessorMacro *macro =
        preprocessorStateFindMacro(
            state, macroName);

    if(macro) {

        macroState = preprocessorStateClone(state);
        if(macroState) {

            preprocessorStateClearOutput(macroState);

            if(executeMacro(macroState, macro, recursionLevel)) {

                // Escape the string and add it to the output.
                escapedStr = escapeString(macroState->output);
                if(escapedStr) {
                    appendString(state, "\"");
                    appendString(state, escapedStr);
                    appendString(state, "\"");
                    freeWrapper(escapedStr);
                }

                // Skip past the stuff we read in the
                // cloned state.
                state->index = macroState->index;

                destroyPreprocessorState(macroState);

                ret = nktrue;
            }
        }

    } else {
        preprocessorStateAddError(
            state,
            "Unknown input for stringification.");
    }

    return ret;
}

// MEMSAFE
nkbool preprocess(
    struct PreprocessorState *state,
    const char *str,
    nkuint32_t recursionLevel)
{
    nkbool ret = nktrue;
    struct PreprocessorToken *token = NULL;
    struct PreprocessorToken *directiveNameToken = NULL;
    char *line = NULL;

    // FIXME: Maybe make this less arbitraty.
    if(recursionLevel > 20) {
        preprocessorStateAddError(state, "Arbitrary recursion limit reached.");
        return nkfalse;
    }

    state->str = str;
    state->index = 0;

    while(state->str && state->str[state->index]) {

        token = getNextToken(state, nktrue);

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
                    directiveNameToken = getNextToken(state, nktrue);

                    if(!directiveNameToken) {

                        ret = nkfalse;

                    } else {

                        // Check to see if this is something we
                        // understand by going through the *actual*
                        // list of directives.
                        if(directiveIsValid(directiveNameToken->str)) {

                            nkuint32_t lineCount = 0;
                            line = readRestOfLine(state, &lineCount);
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
                                    preprocessorStateAddError(
                                        state, "Bad directive.");
                                    ret = nkfalse;
                                }

                                // Clean up.
                                freeWrapper(line);
                                line = NULL;

                            }

                        } else {

                            if(!handleStringification(state, directiveNameToken->str, recursionLevel)) {
                                ret = nkfalse;
                            }
                        }

                        destroyToken(directiveNameToken);
                        directiveNameToken = NULL;

                    }

                } else {

                    // Some non-directive identifier came after the
                    // '#', so we're going to ignore it and just
                    // output it directly as though it's not a
                    // preprocessor directive.
                    if(!appendString(state, token->str)) {
                        ret = nkfalse;
                    }

                }


            } else if(token->type == NK_PPTOKEN_IDENTIFIER) {

                // See if we can find a macro with this name.
                struct PreprocessorMacro *macro =
                    preprocessorStateFindMacro(
                        state, token->str);

                if(macro) {

                    // Execute that macro.
                    if(!executeMacro(state, macro, recursionLevel)) {
                        ret = nkfalse;
                    }

                } else {

                    // This is an identifier, but it's not a defined
                    // macro.
                    if(!appendString(state, token->str)) {
                        ret = nkfalse;
                    }

                }

            } else {

                // We don't know what this is. It's probably not for
                // us. Just pass it through.
                if(!appendString(state, token->str)) {
                    ret = nkfalse;
                }

            }

            destroyToken(token);
            token = NULL;

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
    // 10691?
    // for(nkuint32_t counter = 4200; counter < 2000000; counter++) {
    // for(nkuint32_t counter = 0; counter < 2430; counter++) {
    // for(nkuint32_t counter = 0; counter < 1; counter++) {
    // for(nkuint32_t counter = 18830; counter < 2000000; counter++) {
    for(nkuint32_t counter = 18830; counter < 18831; counter++) {
    // for(nkuint32_t counter = 0; counter < 18831; counter++) {
    // for(nkuint32_t counter = 2432; counter < 18831; counter++) {

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
