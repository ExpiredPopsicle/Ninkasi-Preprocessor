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

/// Use this to initialize an NkppErrorState *before* being used to
/// create an NkppState.
void nkppErrorStateInit(struct NkppErrorState *errorState);

/// Use this to clean up an NkppErrorState *before* destroying the
/// root NkppState that used this. NkppErrorState is not owned by a
/// particular NkppState, but the errors inside were allocated through
/// one, and as such must be deleted before the last NkppState is
/// fully destroyed.
void nkppErrorStateClear(
    struct NkppState *state,
    struct NkppErrorState *errorState);

/// Use this to just dump all the errors to stdout.
void nkppErrorStateDump(
    struct NkppState *state,
    struct NkppErrorState *errorState);

#endif // NK_PPERROR_H
