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
