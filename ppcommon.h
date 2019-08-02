#ifndef NK_PPCOMMON_H
#define NK_PPCOMMON_H

#include <string.h>
#include <malloc.h>

typedef unsigned int nkuint32_t;
typedef int nkint32_t;
typedef unsigned int nkbool;
typedef unsigned char nkuint8_t;
#define nktrue 1
#define nkfalse 0
#define NK_INVALID_VALUE (~(nkuint32_t)0)
#define NK_UINT_MAX (~(nkuint32_t)0)

#define memcpyWrapper(x, y, z) nkiMemcpy(x, y, z)
#define reallocArrayWrapper(x, y, z) reallocWrapper(x, y * z)
#define strlenWrapper(x) strlen(x)
#define strcmpWrapper(x, y) strcmp(x, y)
#define memcpy(x, y, z) dont_use_memcpy_directly

struct PreprocessorState;

nkbool nkiCompilerIsValidIdentifierCharacter(char c, nkbool isFirstCharacter);
nkbool nkiCompilerIsWhitespace(char c);
void nkiCompilerPreprocessorSkipWhitespace(
    const char *str,
    nkuint32_t *k);
nkbool nkiCompilerIsNumber(char c);
void nkiMemcpy(void *dst, const void *src, nkuint32_t len);
void nkiDbgAppendEscaped(nkuint32_t bufSize, char *dst, const char *src);




void *mallocWrapper(nkuint32_t size);
void freeWrapper(void *ptr);

void *nkppRealloc(
    struct PreprocessorState *state,
    void *ptr,
    nkuint32_t size);

char *nkppStrdup(
    struct PreprocessorState *state,
    const char *s);

void setAllocationFailureTestLimits(
    nkuint32_t limitMemory,
    nkuint32_t limitAllocations);

// TODO: FIXME: Replace manual overflow checks with these!
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
