#include "ppcommon.h"

struct NkppState *nkppStateCreate(
    struct NkppMemoryCallbacks *memoryCallbacks)
{
    NkppMallocWrapper mallocWrapper =
        memoryCallbacks && memoryCallbacks->mallocWrapper ?
        memoryCallbacks->mallocWrapper : nkppDefaultMallocWrapper;
    NkppFreeWrapper freeWrapper =
        memoryCallbacks && memoryCallbacks->freeWrapper ?
        memoryCallbacks->freeWrapper : nkppDefaultFreeWrapper;
    void *userData =
        memoryCallbacks ? memoryCallbacks->userData : NULL;

    struct NkppErrorState *errorState = NULL;
    struct NkppState *state = NULL;

    errorState =
        mallocWrapper(NULL, userData, sizeof(struct NkppErrorState));
    if(!errorState) {
        return NULL;
    }

    nkppErrorStateInit(errorState);

    state = nkppStateCreate_internal(errorState, memoryCallbacks);
    if(!state) {
        freeWrapper(NULL, userData, errorState);
        return NULL;
    }

    // This is the outermost instance of the NkppState, and it's the
    // only one that's responsible for writing position markers.
    state->writePositionMarkers = nktrue;

    return state;
}

void nkppStateDestroy(
    struct NkppState *state)
{
    NkppFreeWrapper freeWrapper = NULL;
    void *userData = NULL;

    assert(state);

    freeWrapper =
        state->memoryCallbacks && state->memoryCallbacks->freeWrapper ?
        state->memoryCallbacks->freeWrapper : nkppDefaultFreeWrapper;
    userData =
        state->memoryCallbacks ? state->memoryCallbacks->userData : NULL;

    nkppErrorStateClear(state, state->errorState);

    freeWrapper(NULL, userData, state->errorState);

    nkppStateDestroy_internal(state);
}

nkbool nkppStateExecute(
    struct NkppState *state,
    const char *str,
    const char *filename)
{
    assert(state);

    if(filename) {
        if(!nkppStateSetFilename(state, filename)) {
            return nkfalse;
        }
    }

    return nkppStateExecute_internal(state, str);
}

nkuint32_t nkppStateGetErrorCount(
    const struct NkppState *state)
{
    nkuint32_t count = 0;
    struct NkppError *currentError;

    assert(state);
    assert(state->errorState);

    currentError = state->errorState->firstError;
    while(currentError) {
        count++;
        currentError = currentError->next;
    }

    if(state->errorState->allocationFailure) {
        count++;
    }

    return count;
}

nkbool nkppStateHasError(
    const struct NkppState *state)
{
    assert(state);
    assert(state->errorState);

    if(state->errorState->allocationFailure) {
        return nktrue;
    }

    if(state->errorState->firstError) {
        return nktrue;
    }

    return nkfalse;
}




