#ifndef NK_PPCOMMON_H
#define NK_PPCOMMON_H

#include "pptypes.h"
#include "ppdirect.h"
#include "ppmacro.h"
#include "ppmem.h"
#include "ppstate.h"
#include "ppstring.h"
#include "pptoken.h"

#include <string.h>
#include <malloc.h>
#include <assert.h>

struct NkppState;

void nkppSanityCheck(void);

#define NK_CHECK_OVERFLOW_UINT_ADD(a, b, result, overflow)  \
    do {                                                    \
        if((a) > NK_UINT_MAX - (b)) {                       \
            overflow = nktrue;                              \
        }                                                   \
        result = a + b;                                     \
    } while(0)

#define NK_CHECK_OVERFLOW_UINT_MUL(a, b, result, overflow)  \
    do {                                                    \
        if((a) >= NK_UINT_MAX / (b)) {                      \
            overflow = nktrue;                              \
        }                                                   \
        result = a * b;                                     \
    } while(0)

#endif // NK_PPCOMMON_H
