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

#define NK_PPTEST_CHECK(x)                \
    do {                                        \
        printf("%-70s : ", #x);                 \
        if(!(x)) {                              \
            printf("%s\n", NK_PPTEST_FAIL);     \
            ret = nkfalse;                      \
        } else {                                \
            printf("%s\n", NK_PPTEST_PASS);     \
        }                                       \
    } while(0)

#define NK_PPTEST_PASS "\x1b[1;32mPASS\x1b[0m"
#define NK_PPTEST_FAIL "\x1b[1;31mFAIL\x1b[0m"
#define NK_PPTEST_NULL "\x1b[1;31mNULL\x1b[0m"

nkbool nkppTestRun(void);

#endif // NK_PP_ENABLETESTS

#endif // NK_PPTEST_H

