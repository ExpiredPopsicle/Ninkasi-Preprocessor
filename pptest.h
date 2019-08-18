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

#define NK_PPTEST_PASS "\x1b[1;32mPASS\x1b[0m"
#define NK_PPTEST_FAIL "\x1b[1;31mFAIL\x1b[0m"

nkbool nkppTestRun(void);

#endif // NK_PP_ENABLETESTS

#endif // NK_PPTEST_H

