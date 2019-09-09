// ----------------------------------------------------------------------
//
//   Ninkasi Preprocessor 0.1
//
//   By Kiri "ExpiredPopsicle" Jolly
//     https://expiredpopsicle.com
//     https://intoxicoding.com
//     expiredpopsicle@gmail.com
//
// ----------------------------------------------------------------------
//
//   Copyright (c) 2019 Kiri Jolly
//
//   Permission is hereby granted, free of charge, to any person
//   obtaining a copy of this software and associated documentation files
//   (the "Software"), to deal in the Software without restriction,
//   including without limitation the rights to use, copy, modify, merge,
//   publish, distribute, sublicense, and/or sell copies of the Software,
//   and to permit persons to whom the Software is furnished to do so,
//   subject to the following conditions:
//
//   The above copyright notice and this permission notice shall be
//   included in all copies or substantial portions of the Software.
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
//   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
//   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//   SOFTWARE.
//
// -------------------------- END HEADER -------------------------------------

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
