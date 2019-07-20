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

#define mallocWrapper(x) malloc(x)
#define memcpyWrapper(x, y, z) nkiMemcpy(x, y, z)
#define freeWrapper(x) free(x)
#define reallocWrapper(x, y) realloc(x, y)
#define reallocArrayWrapper(x, y, z) reallocWrapper(x, y * z)
#define strlenWrapper(x) strlen(x)
#define strcmpWrapper(x, y) strcmp(x, y)
#define strdupWrapper(x) strdup(x)
#define memcpy(x, y, z) dont_use_memcpy_directly

nkbool nkiCompilerIsValidIdentifierCharacter(char c, nkbool isFirstCharacter);
nkbool nkiCompilerIsWhitespace(char c);
void nkiCompilerPreprocessorSkipWhitespace(
    const char *str,
    nkuint32_t *k);
nkbool nkiCompilerIsNumber(char c);
void nkiMemcpy(void *dst, const void *src, nkuint32_t len);


#endif // NK_PPCOMMON_H
