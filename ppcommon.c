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

// FIXME: Make MEMSAFE (overflow)
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

// FIXME: Make MEMSAFE (call to nkiDbgAppendEscaped)
char *escapeString(const char *src)
{
    char *output;
    nkuint32_t bufferLen;

    // Return an empty string if input is MULL.
    if(!src) {
        output = mallocWrapper(1);
        if(output) {
            output[0] = 0;
        }
        return output;
    }

    // TODO: Check overflow.
    // bufferLen = strlenWrapper(src) * 2 + 1;
    {
        nkbool overflow = nkfalse;
        bufferLen = strlenWrapper(src);
        NK_CHECK_OVERFLOW_UINT_MUL(bufferLen, 2, bufferLen, overflow);
        NK_CHECK_OVERFLOW_UINT_ADD(bufferLen, 1, bufferLen, overflow);
        if(overflow) {
            return NULL;
        }
    }

    output = mallocWrapper(bufferLen);
    if(!output) {
        return NULL;
    }

    output[0] = 0;
    nkiDbgAppendEscaped(bufferLen, output, src);

    return output;
}


struct AllocHeader
{
    nkuint32_t size;
};

static nkuint32_t maxAllowedMemory = ~(nkuint32_t)0;
static nkuint32_t totalAllocations = 0;
static nkuint32_t maxUsage = 0;
static nkuint32_t allocationsUntilFailure = ~(nkuint32_t)0;

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

void setAllocationFailureTestLimits(
    nkuint32_t limitMemory,
    nkuint32_t limitAllocations)
{
    printf("Alloc limits: %lu bytes, %lu allocs\n",
        (long)limitMemory, (long)limitAllocations);

    maxAllowedMemory = limitMemory;
    allocationsUntilFailure = limitAllocations;
}

void *mallocLowLevel(size_t size)
{
    if(allocationsUntilFailure != ~(nkuint32_t)0) {
        if(allocationsUntilFailure) {
            allocationsUntilFailure--;
        }
    }

    if(!allocationsUntilFailure) {
        return NULL;
    }

    return malloc(size);
}

void *mallocWrapper(nkuint32_t size)
{
    struct AllocHeader *header = NULL;
    void *ret = NULL;

    // Avoid 32-bit to size_t truncation on 16-bit (DOS).
    if(size < ~(size_t)0) {
        header = mallocLowLevel(size + sizeof(struct AllocHeader));
    }

    if(!header) {
        return NULL;
    }

    ret = getAllocHeaderData(header);

    header->size = size;

    totalAllocations += size;
    if(totalAllocations > maxUsage) {
        maxUsage = totalAllocations;
    }

    // printf("Alloc: %8lu ( %8lu )\n",
    //     (long)totalAllocations,
    //     (long)maxUsage);

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

    // printf("Free:  %8lu ( %8lu )\n",
    //     (long)totalAllocations,
    //     (long)maxUsage);
}

void *reallocWrapper(void *ptr, nkuint32_t size)
{
    if(!ptr) {

        return mallocWrapper(size);

    } else {

        void *newChunk = mallocWrapper(size);
        if(!newChunk) {
            return NULL;
        }

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
    if(!ret) {
        return NULL;
    }

    memcpyWrapper(ret, s, len);
    ret[len] = 0;

    return ret;
}
