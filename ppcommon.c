#include "ppcommon.h"
#include "ppstate.h"
#include "ppstring.h"

#include <assert.h>

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
void nkiDbgAppendEscaped(nkuint32_t bufSize, char *dst, const char *src)
{
    nkuint32_t i = nkppStrlen(dst);

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

void nkppSanityCheck(void)
{
    // If any of these fail, you need to find a way to differentiated
    // this platform or data model from the others and put it in
    // pptypes.h.
    assert(sizeof(nkuint32_t) == 4);
    assert(sizeof(nkint32_t) == 4);
    assert(sizeof(nkuint8_t) == 1);
    assert(sizeof(nkbool) == 4);
}



