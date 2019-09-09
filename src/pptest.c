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

#include "ppcommon.h"

#if NK_PP_ENABLETESTS

nkbool nkppTestRun(void)
{
    nkbool ret = nktrue;

    if(!nkppTest_pathTest()) {
        ret = nkfalse;
    }

    if(!nkppTest_expressionTest()) {
        ret = nkfalse;
    }

    #if NK_PP_MEMDEBUG
    printf("----------------------------------------------------------------------\n");
    printf("Memory leaked: %lu\n", (unsigned long)nkppMemDebugGetTotalAllocations());
    if(nkppMemDebugGetTotalAllocations()) {
        ret = nkfalse;
    }
    #endif

    return ret;
}

void nkppTestPrintTestLine(const char *testStr)
{
    nkuint32_t len = nkppStrlen(testStr);
    nkuint32_t i = 0;
    nkuint32_t lineLen = 66;

    // Print out the string, truncated to (lineLen - 4) length.
    while(i < len && i < lineLen - 4) {
        printf("%c", testStr[i]);
        i++;
    }

    // Print out dots if the line was over-length.
    if(i < len) {
        while(i < lineLen - 1) {
            printf(".");
            i++;
        }
    }

    // Pad the rest out with spaces.
    while(i < lineLen) {
        printf(" ");
        i++;
    }
}

#endif // NK_PP_ENABLETESTS
