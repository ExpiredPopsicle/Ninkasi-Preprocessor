#ifndef NK_PPMEM_H
#define NK_PPMEM_H

// ----------------------------------------------------------------------
// Memory functions

void *nkppMalloc(
    struct NkppState *state,
    nkuint32_t size);

void nkppFree(
    struct NkppState *state,
    void *ptr);

void *nkppRealloc(
    struct NkppState *state,
    void *ptr,
    nkuint32_t size);

// ----------------------------------------------------------------------
// Default fallbacks when the user does not specify callbacks

void *nkppDefaultMallocWrapper(
    struct NkppState *state,
    void *userData, nkuint32_t size);

void nkppDefaultFreeWrapper(
    struct NkppState *state,
    void *userData, void *ptr);

char *nkppDefaultLoadFileCallback(
    struct NkppState *state,
    void *userData,
    const char *filename);

// ----------------------------------------------------------------------
// Debugging stuff

#ifndef NK_PP_MEMDEBUG
#define NK_PP_MEMDEBUG 0
#endif

#if NK_PP_MEMDEBUG
void nkppMemDebugSetAllocationFailureTestLimits(
    nkuint32_t limitMemory,
    nkuint32_t limitAllocations);
#endif // NK_PP_MEMDEBUG

#endif // NK_PPMEM_H

