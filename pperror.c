#include "ppcommon.h"

void nkppErrorStateInit(struct NkppErrorState *errorState)
{
    errorState->firstError = NULL;
    errorState->lastError = NULL;
    errorState->allocationFailure = nkfalse;
}

void nkppErrorStateClear(
    struct NkppState *state,
    struct NkppErrorState *errorState)
{
    while(errorState->firstError) {
        struct NkppError *next = errorState->firstError->next;
        nkppFree(state, errorState->firstError->filename);
        nkppFree(state, errorState->firstError->text);
        nkppFree(state, errorState->firstError);
        errorState->firstError = next;
    }
}


