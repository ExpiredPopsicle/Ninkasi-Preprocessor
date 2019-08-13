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



char *nkppStrdup(struct NkppState *state, const char *s)
{
    nkuint32_t len = strlenWrapper(s);
    nkuint32_t size;
    nkbool overflow = nkfalse;
    char *ret;

    NK_CHECK_OVERFLOW_UINT_ADD(len, 1, size, overflow);
    if(overflow) {
        return NULL;
    }

    ret = (char*)nkppMalloc(state, size);
    if(!ret) {
        return NULL;
    }

    memcpyWrapper(ret, s, len);
    ret[len] = 0;

    return ret;
}
