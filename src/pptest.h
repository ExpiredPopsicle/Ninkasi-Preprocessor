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

#ifndef NK_PPTEST_H
#define NK_PPTEST_H

#include "ppconfig.h"

#if NK_PP_ENABLETESTS

#define NK_PPTEST_SECTION(x)                                            \
    do {                                                                \
        printf("----------------------------------------------------------------------\n"); \
        printf("  %s\n", x);                                            \
        printf("----------------------------------------------------------------------\n"); \
    } while(0)

#define NK_PPTEST_CHECK(x)                      \
    do {                                        \
        nkppTestPrintTestLine(#x);              \
        if(!(x)) {                              \
            printf("%s\n", NK_PPTEST_FAIL);     \
            ret = nkfalse;                      \
        } else {                                \
            printf("%s\n", NK_PPTEST_PASS);     \
        }                                       \
    } while(0)

#ifdef __DOS__
#define NK_PPTEST_PASS "PASS"
#define NK_PPTEST_FAIL "FAIL"
#define NK_PPTEST_NULL "NULL"
#else
#define NK_PPTEST_PASS "\x1b[1;32mPASS\x1b[0m"
#define NK_PPTEST_FAIL "\x1b[1;31mFAIL\x1b[0m"
#define NK_PPTEST_NULL "\x1b[1;31mNULL\x1b[0m"
#endif

/// This is just a utility function for our testing macros.
void nkppTestPrintTestLine(const char *testStr);

/// This is the entry point to the test system.
nkbool nkppTestRun(void);

#endif // NK_PP_ENABLETESTS

#endif // NK_PPTEST_H

