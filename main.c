#include "ppcommon.h"

#include <stdio.h>
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


char *loadFile(
    struct NkppState *state,
    void *userData,
    const char *filename)
{
    FILE *in = fopen(filename, "rb");
    nkuint32_t fileSize = 0;
    char *ret;

    // FIXME: Remove this.
    printf("LOADING FILE: %s\n", filename);

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
        nkppFree(state, ret);
        ret = NULL;
    }

    fclose(in);

    return ret;
}

int main(int argc, char *argv[])
{
    nkuint32_t counter;

    // 10691?
    // for(counter = 8022; counter < 2000000; counter++) {
    // for(nkuint32_t counter = 0; counter < 2430; counter++) {
    // for(nkuint32_t counter = 0; counter < 1; counter++) {
    // for(nkuint32_t counter = 18830; counter < 2000000; counter++) {
    // for(counter = 18830; counter < 18831; counter++) {
    // for(nkuint32_t counter = 0; counter < 18831; counter++) {
    // for(nkuint32_t counter = 2432; counter < 18831; counter++) {

    // for(counter = 18830; counter < 18831; counter++) {

    for(counter = 2000000; counter < 2000001; counter++) {

        struct NkppMemoryCallbacks memCallbacks;
        struct NkppErrorState errorState;
        struct NkppState *state;
        char *testStr2;

        // FIXME: Make a real init function for this.
        errorState.firstError = NULL;
        errorState.lastError = NULL;
        errorState.allocationFailure = nkfalse;

        memCallbacks.mallocWrapper = NULL;
        memCallbacks.freeWrapper = NULL;
        memCallbacks.loadFileCallback = loadFile;


        // nkuint32_t allocLimit = ~(nkuint32_t)0;
        // nkuint32_t memLimit = ~(nkuint32_t)0;
        // if(argc > 2) {
        //     memLimit = atoi(argv[2]);
        // }
        // if(argc > 1) {
        //     allocLimit = atol(argv[1]);
        // }
        // nkppMemDebugSetAllocationFailureTestLimits(
        //     memLimit, allocLimit);

        #if NK_PP_MEMDEBUG
        nkppMemDebugSetAllocationFailureTestLimits(
            ~(nkuint32_t)0, counter);
        #endif

        state = nkppStateCreate(&errorState, &memCallbacks);
        if(!state) {
            printf("Allocation failure on state creation.\n");
            // return 0;
            continue;
        }

        testStr2 = loadFile(state, NULL, "test.txt");
        if(!testStr2) {
            printf("Allocation failure on file load.\n");
            nkppStateDestroy(state);
            // return 0;
            continue;
        }

        printf("----------------------------------------------------------------------\n");
        printf("  Input string\n");
        printf("----------------------------------------------------------------------\n");
        printf("%s\n", testStr2);

        {
            printf("----------------------------------------------------------------------\n");
            printf("  Whatever\n");
            printf("----------------------------------------------------------------------\n");

            state->writePositionMarkers = nktrue;
            if(nkppStateExecute(state, testStr2)) {
                printf("Preprocessor success\n");
            } else {
                printf("Preprocessor failed\n");
            }

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


        printf("----------------------------------------------------------------------\n");
        printf("  Iteration: %lu\n", (unsigned long)counter);
        printf("    Allocation failure? %s\n", errorState.allocationFailure ? "yes" : "no");
        printf("----------------------------------------------------------------------\n");

        nkppStateDestroy(state);

        // nkppTest_pathTest();
        nkppTestRun();
    }



    {
        // struct NkppErrorState errorState;
        struct NkppState *state;
        struct NkppState *preprocessExpressionState;
        nkint32_t output = 0;

        // memset(&errorState, 0, sizeof(errorState));

        state = nkppStateCreate(NULL, NULL);
        preprocessExpressionState = nkppStateCreate(NULL, NULL);
        preprocessExpressionState->preprocessingIfExpression = nktrue;

        // nkppStateExecute(
        //     preprocessExpressionState,
        //     "#define junk 1\n"
        //     "defined(junk) + \n"
        //     "junk + \n"
        //     "junk\n");

        // nkppStateExecute(
        //     preprocessExpressionState,
        //     "1 + 2 * 5 * 4 * 2 + 6");

        // nkppStateExecute(
        //     preprocessExpressionState,
        //     "1 + 5 * 2 * 3");

        nkppStateExecute(
            preprocessExpressionState,
            "5 * (1 + 2) * 10");

        printf("Exp preprocessed: %s\n", preprocessExpressionState->output);

        {
            nkbool r =
                nkppEvaluateExpression(preprocessExpressionState, preprocessExpressionState->output, &output, 0);

            // nkppEvaluateExpression(state, "-(~(-(~(-(~100))))) + 2 * (3 + 4)", &output, 0);
            printf("Final expression output (%s): %ld\n", r ? "good" : "bad", (long)output);
        }

        // while(errorState.firstError) {

        //     printf("error: %s:%ld: %s\n",
        //         errorState.firstError->filename,
        //         (long)errorState.firstError->lineNumber,
        //         errorState.firstError->text);

        //     {
        //         struct NkppError *next = errorState.firstError->next;
        //         nkppFree(state, errorState.firstError->filename);
        //         nkppFree(state, errorState.firstError->text);
        //         nkppFree(state, errorState.firstError);
        //         errorState.firstError = next;
        //     }
        // }

        nkppStateDestroy(preprocessExpressionState);
        nkppStateDestroy(state);
    }


    return 0;
}
