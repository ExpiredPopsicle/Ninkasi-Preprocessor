#include "ppcommon.h"
#include "ppstate.h"

// ----------------------------------------------------------------------
// Stuff copied from nktoken.c

// MEMSAFE
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

// MEMSAFE
nkbool nkiCompilerIsWhitespace(char c)
{
    if(c == ' ' || c == '\n' || c == '\t' || c == '\r') {
        return nktrue;
    }
    return nkfalse;
}

// void nkiCompilerPreprocessorSkipWhitespace(
//     const char *str,
//     nkuint32_t *k)
// {
//     while(str[*k] && nkiCompilerIsWhitespace(str[*k])) {

//         // Bail out when we get to the end of the line.
//         if(str[*k] == '\n') {
//             break;
//         }

//         (*k)++;
//     }
// }

// MEMSAFE
nkbool nkiCompilerIsNumber(char c)
{
    return (c >= '0' && c <= '9');
}

// MEMSAFE
void nkiMemcpy(void *dst, const void *src, nkuint32_t len)
{
    nkuint32_t i;
    for(i = 0; i < len; i++) {
        ((char*)dst)[i] = ((const char*)src)[i];
    }
}

// MEMSAFE
void nkiDbgAppendEscaped(nkuint32_t bufSize, char *dst, const char *src)
{
    nkuint32_t i = strlenWrapper(dst);

    // Avoid overflows at the cost of truncating strings near the end.
    if(i > NK_UINT_MAX - 3) {
        return;
    }

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

        // Avoid overflows at the cost of truncating strings near the
        // end.
        if(i > NK_UINT_MAX - 3) {
            break;
        }

    }

    dst[i] = 0;
}

// MEMSAFE
char *escapeString(
    struct PreprocessorState *state,
    const char *src)
{
    char *output;
    nkuint32_t bufferLen;
    nkbool overflow = nkfalse;

    // Return an empty string if input is MULL.
    if(!src) {
        output = nkppMalloc(state, 1);
        if(output) {
            output[0] = 0;
        }
        return output;
    }

    // Just make a buffer that's twice as big in case literally every
    // single character needs to be escaped.
    bufferLen = strlenWrapper(src);
    NK_CHECK_OVERFLOW_UINT_MUL(bufferLen, 2, bufferLen, overflow);
    NK_CHECK_OVERFLOW_UINT_ADD(bufferLen, 1, bufferLen, overflow);
    if(overflow) {
        return NULL;
    }

    output = nkppMalloc(state, bufferLen);
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

// MEMSAFE
void *reallocWrapper(void *ptr, nkuint32_t size)
{
    if(!ptr) {

        return mallocWrapper(size);

    } else {

        struct AllocHeader *oldHeader;
        nkuint32_t oldSize;

        void *newChunk = mallocWrapper(size);
        if(!newChunk) {
            return NULL;
        }

        oldHeader = getAllocHeader(ptr);
        oldSize = oldHeader->size;

        memcpyWrapper(newChunk, ptr, size > oldSize ? oldSize : size);

        freeWrapper(ptr);

        return newChunk;
    }
}

char *nkppStrdup(struct PreprocessorState *state, const char *s)
{
    return strdupWrapper(s);
}

// FIXME: Remove this.
char *strdupWrapper(const char *s)
{
    nkuint32_t len = strlenWrapper(s);
    nkuint32_t size;
    nkbool overflow = nkfalse;
    char *ret;

    NK_CHECK_OVERFLOW_UINT_ADD(len, 1, size, overflow);
    if(overflow) {
        return NULL;
    }

    // ret = (char*)nkppMalloc(state, size);
    ret = (char*)mallocWrapper(size);
    if(!ret) {
        return NULL;
    }

    memcpyWrapper(ret, s, len);
    ret[len] = 0;

    return ret;
}
