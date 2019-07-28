#include "ppcommon.h"

// ----------------------------------------------------------------------
// Stuff copied from nktoken.c

nkbool nkiCompilerIsValidIdentifierCharacter(char c, nkbool isFirstCharacter)
{
    if(!isFirstCharacter) {
        if(c >= '0' && c <= '9') {
            return nktrue;
        }
    }

    if(c == '_') return nktrue;
    if(c >= 'a' && c <= 'z') return nktrue;
    if(c >= 'A' && c <= 'Z') return nktrue;

    return nkfalse;
}

nkbool nkiCompilerIsWhitespace(char c)
{
    if(c == ' ' || c == '\n' || c == '\t' || c == '\r') {
        return nktrue;
    }
    return nkfalse;
}

void nkiCompilerPreprocessorSkipWhitespace(
    const char *str,
    nkuint32_t *k)
{
    while(str[*k] && nkiCompilerIsWhitespace(str[*k])) {

        // Bail out when we get to the end of the line.
        if(str[*k] == '\n') {
            break;
        }

        (*k)++;
    }
}

nkbool nkiCompilerIsNumber(char c)
{
    return (c >= '0' && c <= '9');
}

void nkiMemcpy(void *dst, const void *src, nkuint32_t len)
{
    nkuint32_t i;
    for(i = 0; i < len; i++) {
        ((char*)dst)[i] = ((const char*)src)[i];
    }
}

void nkiDbgAppendEscaped(nkuint32_t bufSize, char *dst, const char *src)
{
    nkuint32_t i = strlenWrapper(dst);

    // TODO: Check for overflow.

    while(i + 2 < bufSize && *src) {
        switch(*src) {
            case '\n':
                dst[i++] = '\\';
                dst[i++] = 'n';
                break;
            case '\t':
                dst[i++] = '\\';
                dst[i++] = 't';
                break;
            case '\"':
                dst[i++] = '\\';
                dst[i++] = '\"';
                break;
            default:
                dst[i++] = *src;
                break;
        }
        src++;
    }
    dst[i] = 0;
}

char *escapeString(const char *src)
{
    char *output;
    nkuint32_t bufferLen;

    if(!src) {
        return NULL;
    }

    // TODO: Check overflow.
    bufferLen = strlenWrapper(src) * 2 + 1;

    output = mallocWrapper(bufferLen);
    output[0] = 0;
    nkiDbgAppendEscaped(bufferLen, output, src);

    return output;
}


struct AllocHeader
{
    nkuint32_t size;
};

static nkuint32_t totalAllocations = 0;
static nkuint32_t maxUsage = 0;

struct AllocHeader *getAllocHeader(void *ptr)
{
    struct AllocHeader *header = ((struct AllocHeader*)ptr) - 1;
    return header;
}

struct AllocHeader *getAllocHeaderData(struct AllocHeader *header)
{
    void *ret = header + 1;
    return ret;
}

void *mallocWrapper(nkuint32_t size)
{
    struct AllocHeader *header =
        malloc(size + sizeof(struct AllocHeader));

    void *ret = getAllocHeaderData(header);

    header->size = size;

    totalAllocations += size;
    if(totalAllocations > maxUsage) {
        maxUsage = totalAllocations;
    }

    printf("Alloc: %8lu ( %8lu )\n",
        (long)totalAllocations,
        (long)maxUsage);

    return ret;
}

void freeWrapper(void *ptr)
{
    if(!ptr) {
        return;
    } else {
        struct AllocHeader *header = getAllocHeader(ptr);
        totalAllocations -= header->size;
        free(header);
    }

    printf("Free:  %8lu ( %8lu )\n",
        (long)totalAllocations,
        (long)maxUsage);
}

void *reallocWrapper(void *ptr, nkuint32_t size)
{
    if(!ptr) {

        return mallocWrapper(size);

    } else {

        void *newChunk = mallocWrapper(size);
        struct AllocHeader *oldHeader = getAllocHeader(ptr);
        nkuint32_t oldSize = oldHeader->size;

        memcpyWrapper(newChunk, ptr, size > oldSize ? oldSize : size);

        // free(oldHeader);
        freeWrapper(ptr);

        return newChunk;
    }
}

char *strdupWrapper(const char *s)
{
    // FIXME: Check overflow.
    nkuint32_t len = strlenWrapper(s);
    nkuint32_t size = len + 1;

    char *ret = (char*)mallocWrapper(size);

    memcpyWrapper(ret, s, len);
    ret[len] = 0;

    return ret;
}
