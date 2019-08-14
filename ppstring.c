#include "ppcommon.h"

char *nkppDeleteBackslashNewlines(
    struct NkppState *state,
    const char *str)
{
    if(str) {

        nkuint32_t inputLen = nkppStrlen(str);
        nkuint32_t outputLen;
        char *outputStr;
        nkbool overflow = nkfalse;

        NK_CHECK_OVERFLOW_UINT_ADD(inputLen, 1, outputLen, overflow);

        if(!overflow) {

            outputStr = nkppMalloc(state, outputLen);

            if(outputStr) {

                nkuint32_t i;
                nkuint32_t n;

                n = 0;
                for(i = 0; i < inputLen; i++) {

                    // Skip backslashes before a newline.
                    if(str[i] == '\\' && str[i+1] == '\n') {
                        i++;
                    }

                    outputStr[n++] = str[i];
                }

                outputStr[n] = 0;

                return outputStr;
            }
        }
    }

    return NULL;
}

char *nkppStripCommentsAndTrim(
    struct NkppState *state,
    const char *in)
{
    char *ret;
    nkuint32_t readIndex = 0;
    nkuint32_t writeIndex = 0;
    nkuint32_t bufLen = 0;
    nkbool overflow = nkfalse;

    if(!in) {
        return NULL;
    }

    NK_CHECK_OVERFLOW_UINT_ADD(nkppStrlen(in), 1, bufLen, overflow);
    if(overflow) {
        return NULL;
    }

    ret = nkppMalloc(state, bufLen);
    if(!ret) {
        return NULL;
    }

    // Skip whitespace on the start.
    while(in[readIndex] && nkppIsWhitespace(in[readIndex])) {
        readIndex++;
    }

    while(in[readIndex]) {

        if(in[readIndex] == '"') {

            // If we see the start of a string, run through to the end
            // of the string.

            nkbool escaped = nkfalse;

            // Skip initial '"'.
            ret[writeIndex++] = in[readIndex++];

            while(in[readIndex]) {

                if(!escaped && in[readIndex] == '"') {

                    ret[writeIndex++] = in[readIndex++];
                    break;

                } else if(in[readIndex] == '\\') {

                    escaped = !escaped;

                } else {

                    escaped = nkfalse;

                }

                ret[writeIndex++] = in[readIndex++];
            }

        } else if(in[readIndex] == '/' && in[readIndex + 1] == '/') {

            // If we see a comment, run through to the end of the line.
            while(in[readIndex] && in[readIndex] != '\n') {
                readIndex++;
            }

        } else if(in[readIndex] == '/' && in[readIndex + 1] == '*') {

            // C-style comment. Run through until we hit the end or a "*/".

            readIndex += 2; // Skip initial comment maker.

            while(in[readIndex] && in[readIndex+1]) {
                if(in[readIndex] == '*' && in[readIndex + 1] == '/') {
                    readIndex += 2;
                    break;
                }
                readIndex++;
            }

        } else {

            // Just write the character.
            ret[writeIndex++] = in[readIndex++];

        }
    }

    // Back up the write index until we find some non-whitespace.
    while(writeIndex) {
        if(!nkppIsWhitespace(ret[writeIndex-1])) {
            break;
        }
        writeIndex--;
    }

    ret[writeIndex] = 0;
    return ret;
}

char *nkppEscapeString(
    struct NkppState *state,
    const char *src)
{
    char *output;
    nkuint32_t bufferLen;
    nkbool overflow = nkfalse;
    nkuint32_t i = 0;

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
    bufferLen = nkppStrlen(src);
    NK_CHECK_OVERFLOW_UINT_MUL(bufferLen, 2, bufferLen, overflow);
    NK_CHECK_OVERFLOW_UINT_ADD(bufferLen, 1, bufferLen, overflow);
    if(overflow) {
        return NULL;
    }

    // Do the allocation.
    output = nkppMalloc(state, bufferLen);
    if(!output) {
        return NULL;
    }

    // Fill it in.
    while(i + 2 < bufferLen && *src) {
        switch(*src) {
            case '\n':
                output[i++] = '\\';
                output[i++] = 'n';
                break;
            case '\t':
                output[i++] = '\\';
                output[i++] = 't';
                break;
            case '\"':
                output[i++] = '\\';
                output[i++] = '\"';
                break;
            default:
                output[i++] = *src;
                break;
        }
        src++;

        // Avoid overflows at the cost of truncating strings near the
        // end.
        if(i > NK_UINT_MAX - 3) {
            break;
        }

    }

    // Finish the string off.
    output[i] = 0;

    return output;
}

char *nkppStrdup(struct NkppState *state, const char *s)
{
    nkuint32_t len = nkppStrlen(s);
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

    nkppMemcpy(ret, s, len);
    ret[len] = 0;

    return ret;
}

void nkppMemcpy(void *dst, const void *src, nkuint32_t len)
{
    nkuint32_t i;
    for(i = 0; i < len; i++) {
        ((char*)dst)[i] = ((const char*)src)[i];
    }
}

nkuint32_t nkppStrlen(const char *str)
{
    nkuint32_t ret = 0;

    if(!str) {
        return 0;
    }

    while(str[0]) {

        if(ret == NK_UINT_MAX) {
            return NK_UINT_MAX;
        }

        ret++;
        str++;
    }

    return ret;
}

int nkppStrcmp(const char *a, const char *b)
{
    int ret = 0;
    nkuint32_t i = 0;

    while(i != NK_UINT_MAX) {

        if(a[i] < b[i]) {
            ret = -1;
            break;
        }

        if(b[i] < a[i]) {
            ret = 1;
            break;
        }

        if(!a[i]) {
            break;
        }

        i++;
    }

    return ret;
}

nkbool nkppIsWhitespace(char c)
{
    if(c == ' ' || c == '\n' || c == '\t' || c == '\r') {
        return nktrue;
    }
    return nkfalse;
}

nkbool nkppIsValidIdentifierCharacter(char c, nkbool isFirstCharacter)
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

nkbool nkppIsDigit(char c)
{
    return (c >= '0' && c <= '9');
}

nkbool nkppIsDigitHex(char c)
{
    return (c >= '0' && c <= '9') ||
        (c >= 'a' && c <= 'f') ||
        (c >= 'A' && c <= 'F');
}

nkuint32_t nkppParseDigit(char c)
{
    if(c >= '0' && c <= '9') {
        return c - '0';
    }

    if(c >= 'a' && c <= 'z') {
        return 10 + (c - 'a');
    }

    if(c >= 'A' && c <= 'Z') {
        return 10 + (c - 'A');
    }

    return NK_INVALID_VALUE;
}

nkbool nkppStrtol(const char *str, nkuint32_t *out)
{
    nkuint32_t base = 10;

    *out = 0;

    if(!str || !str[0]) {
        return nkfalse;
    }

    // Skip whitespace.
    while(nkppIsWhitespace(str[0])) {
        str++;
    }

    // Determine base.
    if(str[0] == '0') {

        // Default to octal with no other info.
        base = 8;
        str++;

        if(str[0] == 'x') {

            // Hex.
            base = 16;
            str++;

        } else if(str[0] == 'b') {

            // Binary.
            base = 2;
            str++;

        }
    }

    while(str[0]) {

        nkuint32_t digit = nkppParseDigit(str[0]);

        if(digit >= base || digit == NK_INVALID_VALUE) {
            return nkfalse;
        }

        *out = *out * base;
        *out += digit;

        str++;
    }

    return nktrue;
}
