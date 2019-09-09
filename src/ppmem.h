#ifndef NK_PPMEM_H
#define NK_PPMEM_H

#include "ppconfig.h"

// ----------------------------------------------------------------------
// Default fallbacks when the user does not specify callbacks

void *nkppDefaultMallocWrapper(
    void *userData, nkuint32_t size);

void nkppDefaultFreeWrapper(
    void *userData, void *ptr);

char *nkppDefaultLoadFileCallback(
    struct NkppState *state,
    void *userData,
    const char *filename,
    nkbool systemInclude);

// ----------------------------------------------------------------------
// Debugging stuff

#if NK_PP_MEMDEBUG

void nkppMemDebugSetAllocationFailureTestLimits(
    nkuint32_t limitMemory,
    nkuint32_t limitAllocations);

nkuint32_t nkppMemDebugGetTotalAllocations(void);

#endif // NK_PP_MEMDEBUG

#endif // NK_PPMEM_H

