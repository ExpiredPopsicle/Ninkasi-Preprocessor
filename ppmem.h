#ifndef NK_PPMEM_H
#define NK_PPMEM_H

// ----------------------------------------------------------------------
// Memory functions

void *nkppDefaultMallocWrapper(void *userData, nkuint32_t size);

void nkppDefaultFreeWrapper(void *userData, void *ptr);

void *nkppRealloc(
    struct NkppState *state,
    void *ptr,
    nkuint32_t size);

// ----------------------------------------------------------------------
// Debugging stuff

#ifndef NK_PP_MEMDEBUG
#define NK_PP_MEMDEBUG 1
#endif

#if NK_PP_MEMDEBUG
void nkppMemDebugSetAllocationFailureTestLimits(
    nkuint32_t limitMemory,
    nkuint32_t limitAllocations);
#endif // NK_PP_MEMDEBUG

#endif // NK_PPMEM_H

