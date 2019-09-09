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

