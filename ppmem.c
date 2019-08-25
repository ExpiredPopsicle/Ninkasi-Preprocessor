#include "ppcommon.h"

// ----------------------------------------------------------------------
// Allocation header stuff

struct NkppAllocationHeader
{
    nkuint32_t size;
};

struct NkppAllocationHeader *nkppAllocationHeaderGetFromData(void *ptr)
{
    struct NkppAllocationHeader *header = ((struct NkppAllocationHeader*)ptr) - 1;
    return header;
}

struct NkppAllocationHeader *nkppAllocationHeaderGetData(struct NkppAllocationHeader *header)
{
    void *ret = header + 1;
    return ret;
}

// ----------------------------------------------------------------------
// Allocation failure handling debugging

#if NK_PP_MEMDEBUG

static nkuint32_t nkppMemDebugMaxAllowedMemory = NK_UINT_MAX;
static nkuint32_t nkppMemDebugTotalAllocations = 0;
static nkuint32_t nkppMemDebugMaxUsage = 0;
static nkuint32_t nkppMemDebugAllocationsUntilFailure = NK_INVALID_VALUE;

void nkppMemDebugSetAllocationFailureTestLimits(
    nkuint32_t limitMemory,
    nkuint32_t limitAllocations)
{
    printf("Alloc limits: %lu bytes, %lu allocs\n",
        (long)limitMemory, (long)limitAllocations);

    nkppMemDebugMaxAllowedMemory = limitMemory;
    nkppMemDebugAllocationsUntilFailure = limitAllocations;
}

nkuint32_t nkppMemDebugGetTotalAllocations(void)
{
    return nkppMemDebugTotalAllocations;
}

#endif

// ----------------------------------------------------------------------
// malloc/free/etc as used by the VM

void *nkppRealloc(
    struct NkppState *state,
    void *ptr,
    nkuint32_t size)
{
    if(!ptr) {

        return nkppMalloc(state, size);

    } else {

        struct NkppAllocationHeader *oldHeader;
        nkuint32_t oldSize;

        void *newChunk = nkppMalloc(state, size);
        if(!newChunk) {
            return NULL;
        }

        oldHeader = nkppAllocationHeaderGetFromData(ptr);
        oldSize = oldHeader->size;

        nkppMemcpy(newChunk, ptr, size > oldSize ? oldSize : size);

        nkppFree(state, ptr);

        return newChunk;
    }
}

void *nkppMalloc(struct NkppState *state, nkuint32_t size)
{
    void *ret = NULL;
    nkuint32_t actualSize;
    nkbool overflow = nkfalse;
    struct NkppAllocationHeader *newHeader = NULL;

    NK_CHECK_OVERFLOW_UINT_ADD(
        size, sizeof(struct NkppAllocationHeader),
        actualSize, overflow);
    if(overflow) {
        if(state->errorState) {
            state->errorState->allocationFailure = nktrue;
        }
        return NULL;
    }

#if NK_PP_MEMDEBUG
    if(nkppMemDebugAllocationsUntilFailure) {
        nkppMemDebugAllocationsUntilFailure--;
    } else {
        if(state->errorState) {
            state->errorState->allocationFailure = nktrue;
        }
        return NULL;
    }
#endif // NK_PP_MEMDEBUG

    if(state->memoryCallbacks && state->memoryCallbacks->mallocWrapper) {
        newHeader = state->memoryCallbacks->mallocWrapper(
            state, state->memoryCallbacks->userData, actualSize);
    } else {
        newHeader = nkppDefaultMallocWrapper(state, NULL, actualSize);
    }

    if(!newHeader) {
        if(state->errorState) {
            state->errorState->allocationFailure = nktrue;
        }
        return NULL;
    }

    newHeader->size = size;

    ret = nkppAllocationHeaderGetData(newHeader);

#if NK_PP_MEMDEBUG
    nkppMemDebugTotalAllocations += size;
    if(nkppMemDebugTotalAllocations > nkppMemDebugMaxUsage) {
        nkppMemDebugMaxUsage = nkppMemDebugTotalAllocations;
    }
#endif // NK_PP_MEMDEBUG

    return ret;
}

void nkppFree(struct NkppState *state, void *ptr)
{
    struct NkppAllocationHeader *header = NULL;

    if(!ptr) {
        return;
    }

    header = nkppAllocationHeaderGetFromData(ptr);

#if NK_PP_MEMDEBUG
    nkppMemDebugTotalAllocations -= header->size;
#endif // NK_PP_MEMDEBUG

    if(state->memoryCallbacks && state->memoryCallbacks->freeWrapper) {
        state->memoryCallbacks->freeWrapper(
            state, state->memoryCallbacks->userData, header);
    } else {
        nkppDefaultFreeWrapper(state, NULL, header);
    }
}

// ----------------------------------------------------------------------
// Default handlers (lowest level before actual system calls)

void *nkppDefaultMallocWrapper(
    struct NkppState *state,
    void *userData,
    nkuint32_t size)
{
    if(size > ~(size_t)0) {
        return NULL;
    }
    return malloc(size);
}

void nkppDefaultFreeWrapper(
    struct NkppState *state,
    void *userData,
    void *ptr)
{
    free(ptr);
}

char *nkppDefaultLoadFileCallback(
    struct NkppState *state,
    void *userData,
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
        nkppFree(state, ret);
        ret = NULL;
    }

    fclose(in);

    return ret;
}


