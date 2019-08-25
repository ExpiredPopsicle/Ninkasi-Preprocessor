#ifndef NK_PPERROR_H
#define NK_PPERROR_H

#include "pptypes.h"

struct NkppError
{
    char *filename;
    nkuint32_t lineNumber;
    char *text;
    struct NkppError *next;
};

struct NkppErrorState
{
    struct NkppError *firstError;
    struct NkppError *lastError;

    // Set to nktrue if there was an allocation failure at any point
    // from a nkppMalloc() call on this state.
    nkbool allocationFailure;
};

#endif // NK_PPERROR_H
