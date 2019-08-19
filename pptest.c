#include "ppcommon.h"

#if NK_PP_ENABLETESTS

nkbool nkppTestRun(void)
{
    nkbool ret = nktrue;
    if(!nkppTest_pathTest()) {
        ret = nkfalse;
    }
    return ret;
}

void nkppTestPrintTestLine(const char *testStr)
{
    nkuint32_t len = nkppStrlen(testStr);
    nkuint32_t i = 0;
    nkuint32_t lineLen = 66;

    // Print out the string, truncated to (lineLen - 4) length.
    while(i < len && i < lineLen - 4) {
        printf("%c", testStr[i]);
        i++;
    }

    // Print out dots if the line was over-length.
    if(i < len) {
        while(i < lineLen - 1) {
            printf(".");
            i++;
        }
    }

    // Pad the rest out with spaces.
    while(i < lineLen) {
        printf(" ");
        i++;
    }
}

#endif // NK_PP_ENABLETESTS
