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
static nkuint32_t allocationsUntilFailure = NK_INVALID_VALUE;

void nkppMemDebugSetAllocationFailureTestLimits(
    nkuint32_t limitMemory,
    nkuint32_t limitAllocations)
{
    printf("Alloc limits: %lu bytes, %lu allocs\n",
        (long)limitMemory, (long)limitAllocations);

    nkppMemDebugMaxAllowedMemory = limitMemory;
    allocationsUntilFailure = limitAllocations;
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

    if(state->memoryCallbacks) {
        newHeader = state->memoryCallbacks->mallocWrapper(
            state->memoryCallbacks->userData, actualSize);
    } else {
        newHeader = nkppDefaultMallocWrapper(NULL, actualSize);
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

    if(state->memoryCallbacks) {
        state->memoryCallbacks->freeWrapper(
            state->memoryCallbacks->userData, header);
    } else {
        nkppDefaultFreeWrapper(NULL, header);
    }
}

// ----------------------------------------------------------------------
// Default handlers (lowest level before actual system calls)

void *nkppDefaultMallocWrapper(void *userData, nkuint32_t size)
{
    if(size > ~(size_t)0) {
        return NULL;
    }
    return malloc(size);
}

void nkppDefaultFreeWrapper(void *userData, void *ptr)
{
    free(ptr);
}

