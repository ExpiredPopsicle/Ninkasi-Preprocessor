// "ppx.h" is the public interface to the preprocessor. It should be
// all a hosting application needs to use the preprocessor.
#include "ppx.h"

#include <stdio.h>

int main(int argc, char *argv[])
{
    // Main state and state creation data.
    struct NkppState *state = NULL;
    struct NkppMemoryCallbacks memCallbacks = {0};

    // Pointer to a buffer for the loaded file.
    char *fileData = NULL;

    // Error tracking stuff.
    nkuint32_t errorCount = 0;
    nkuint32_t i = 0;

    // Pointer to a buffer for the preprocessed data.
    const char *output = NULL;

    // Check command line arguments.
    if(argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // Create the preprocessor state.
    state = nkppStateCreate(&memCallbacks);
    if(!state) {
        fprintf(stderr, "error: Failed to create preprocessor state.\n");
        return 1;
    }

    // Load a file.
    fileData = nkppSimpleLoadFile(state, argv[1]);
    if(!fileData) {
        fprintf(stderr, "error: Failed to read file: %s\n", argv[1]);
        return 1;
    }

    // Run the preprocessor.
    if(nkppStateExecute(state, fileData, argv[1])) {

        // Preprocessor successful. Print output.
        output = nkppStateGetOutput(state);
        if(output) {
            printf("%s\n", output);
        }

    } else {

        // Had some errors. Dump them to stderr.
        errorCount = nkppStateGetErrorCount(state);
        for(i = 0; i < errorCount; i++) {
            const char *text = NULL;
            const char *fname = NULL;
            nkuint32_t lineNumber = 0;
            if(nkppStateGetError(state, i, &fname, &lineNumber, &text)) {
                fprintf(stderr, "error: %s:%lu: %s\n",
                    fname, (unsigned long)lineNumber, text);
            } else {
                fprintf(stderr, "error: Can't read error message.\n");
            }
        }
    }

    // Clean up.
    nkppStateDestroy(state);

    return 0;
}

