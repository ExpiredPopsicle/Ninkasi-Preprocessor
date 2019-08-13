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

        macroState = nkppStateClone(state, nkfalse);
        if(macroState) {

            if(nkppMacroExecute(macroState, macro, recursionLevel)) {

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

nkbool nkppStateExecute(
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
                        if(nkppDirectiveIsValid(directiveNameToken->str)) {

                            nkuint32_t lineCount = 0;

                            line = nkppStateInputReadRestOfLine(
                                state, &lineCount);

                            if(!line) {

                                ret = nkfalse;

                            } else {

                                if(nkppDirectiveHandleDirective(
                                        state,
                                        directiveNameToken->str,
                                        line))
                                {

                                    // That went well.

                                } else {

                                    // I don't know if we really need
                                    // to report an error here,
                                    // because an error would have
                                    // been added in nkppDirectiveHandleDirective()
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

                        nkppTokenDestroy(state, directiveNameToken);
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
                    if(!nkppMacroExecute(state, macro, recursionLevel)) {
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

            nkppTokenDestroy(state, token);
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

    if(!in) {
        return NULL;
    }

    fseek(in, 0, SEEK_END);
    fileSize = ftell(in);
    fseek(in, 0, SEEK_SET);

    ret = nkppMalloc(state, fileSize + 1);
    if(!ret) {
        fclose(in);
        return NULL;
    }

    ret[fileSize] = 0;

    if(!fread(ret, fileSize, 1, in)) {
        free(ret);
        ret = NULL;
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

        #if NK_PP_MEMDEBUG
        setAllocationFailureTestLimits(
            ~(nkuint32_t)0, counter);
        #endif

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
            nkppStateExecute(state, testStr2, 0);

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
