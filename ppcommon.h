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

#define memcpyWrapper(x, y, z) nkiMemcpy(x, y, z)
#define reallocArrayWrapper(x, y, z) reallocWrapper(x, y * z)
#define strlenWrapper(x) strlen(x)
#define strcmpWrapper(x, y) strcmp(x, y)
#define memcpy(x, y, z) dont_use_memcpy_directly

nkbool nkiCompilerIsValidIdentifierCharacter(char c, nkbool isFirstCharacter);
nkbool nkiCompilerIsWhitespace(char c);
void nkiCompilerPreprocessorSkipWhitespace(
    const char *str,
    nkuint32_t *k);
nkbool nkiCompilerIsNumber(char c);
void nkiMemcpy(void *dst, const void *src, nkuint32_t len);
void nkiDbgAppendEscaped(nkuint32_t bufSize, char *dst, const char *src);
char *escapeString(const char *src);



// #define mallocWrapper(x) malloc(x)
// #define freeWrapper(x) free(x)
// #define reallocWrapper(x, y) realloc(x, y)
// #define strdupWrapper(x) strdup(x)

void *mallocWrapper(nkuint32_t size);
void freeWrapper(void *ptr);
void *reallocWrapper(void *ptr, nkuint32_t size);
char *strdupWrapper(const char *s);


#endif // NK_PPCOMMON_H
