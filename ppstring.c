#include "ppstring.h"

// MEMSAFE
char *deleteBackslashNewlines(const char *str)
{
    if(str) {

        nkuint32_t inputLen = strlenWrapper(str);
        nkuint32_t outputLen;
        char *outputStr;
        nkbool overflow = nkfalse;

        NK_CHECK_OVERFLOW_UINT_ADD(inputLen, 1, outputLen, overflow);

        if(!overflow) {

            outputStr = mallocWrapper(outputLen);

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

// FIXME: Make MEMSAFE (overflow)
char *stripCommentsAndTrim(const char *in)
{
    char *ret;
    nkuint32_t readIndex = 0;
    nkuint32_t writeIndex = 0;

    if(!in) {
        return NULL;
    }

    // FIXME: Check overflow.
    ret = mallocWrapper(strlenWrapper(in) + 1);
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
