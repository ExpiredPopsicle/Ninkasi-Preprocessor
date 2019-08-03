#include "ppstate.h"
#include "ppstring.h"

char *nkppDeleteBackslashNewlines(
    struct NkppState *state,
    const char *str)
{
    if(str) {

        nkuint32_t inputLen = strlenWrapper(str);
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

    NK_CHECK_OVERFLOW_UINT_ADD(strlenWrapper(in), 1, bufLen, overflow);
    if(overflow) {
        return NULL;
    }

    ret = nkppMalloc(state, bufLen);
    if(!ret) {
        return NULL;
    }

    // Skip whitespace on the start.
    while(in[readIndex] && nkiCompilerIsWhitespace(in[readIndex])) {
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
        if(!nkiCompilerIsWhitespace(ret[writeIndex-1])) {
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
