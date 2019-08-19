#include "ppcommon.h"

nkbool nkppPathCharIsSeparator(char c)
{
    return c == '/' || c == '\\';
}


// Returns NK_UINT_MAX if there is no slash.
nkuint32_t nkppPathFindLastSlash(
    const char *path)
{
    nkuint32_t i = nkppStrlen(path);

    // Skip trailing '/'s.
    if(i > 1) {
        i--;
        while(i && nkppPathCharIsSeparator(path[i])) {
            i--;
        }
    }

    // Seek backwards from the beginning.
    while(i != NK_UINT_MAX) {
        if(nkppPathCharIsSeparator(path[i])) {
            break;
        }
        i--;
    }

    return i;
}

char *nkppPathDirname(
    struct NkppState *state,
    const char *path)
{
    nkuint32_t i = nkppPathFindLastSlash(path);
    char *ret = NULL;

    // Made it all the way to the beginning of the string without
    // finding a directory separator. Must be a filename in this
    // directory.
    if(i == NK_UINT_MAX) {
        return nkppStrdup(state, ".");
    }

    // Made it all the way to the beginning of the string and then
    // found a directory separator. Must be in the root directory.
    if(i == 0) {
        return nkppStrdup(state, "/");
    }

    // Return just the slice of the string with the directory name.
    ret = nkppMalloc(state, i+1);
    if(ret) {
        nkppMemcpy(ret, path, i);
        ret[i] = 0;
    }

    return ret;
}

char *nkppPathBasename(
    struct NkppState *state,
    const char *path)
{
    nkuint32_t i = nkppPathFindLastSlash(path);
    nkuint32_t len = nkppStrlen(path);
    char *ret = NULL;
    nkuint32_t retLen = 0;

    if(i == NK_UINT_MAX) {

        // Made it all the way to the beginning of the string without
        // finding a directory separator. Must be a filename in this
        // directory.
        ret = nkppStrdup(state, path);

    } else if(i == 0) {

        // Made it all the way to the beginning of the string and then
        // found a directory separator. Must be in the root directory.
        ret = nkppStrdup(state, path + 1);

    } else {

        // Return just the slice of the string with the file name.
        ret = nkppMalloc(state, len - i + 1);
        if(ret) {
            nkppMemcpy(ret, path + i + 1, len - i);
            ret[len - i] = 0;
        }
    }

    // Remove trailing slashes.
    retLen = nkppStrlen(ret);
    while(retLen > 1 && nkppPathCharIsSeparator(ret[retLen - 1])) {
        ret[retLen - 1] = 0;
        retLen--;
    }

    return ret;
}

struct NkppPathTidyPathTokens
{
    nkuint32_t tokenCount;
    char **tokens;
};

nkbool nkppPathTidyPath_addToken(
    struct NkppState *state,
    char *token,
    struct NkppPathTidyPathTokens *tokenList)
{
    char **newTokenList = NULL;
    nkbool overflow = nkfalse;
    nkuint32_t newTokenCount;

    NK_CHECK_OVERFLOW_UINT_ADD(tokenList->tokenCount, 1, newTokenCount, overflow);
    if(overflow) {
        return nkfalse;
    }

    newTokenList = nkppRealloc(
        state,
        tokenList->tokens,
        sizeof(char*) * newTokenCount);
    if(!newTokenList) {
        return nkfalse;
    }

    tokenList->tokens = newTokenList;
    tokenList->tokenCount = newTokenCount;

    tokenList->tokens[tokenList->tokenCount - 1] = token;

    return nktrue;
}

void nkppPathTidyPath_deleteToken(
    struct NkppPathTidyPathTokens *tokenList,
    nkuint32_t index)
{
    nkuint32_t i;

    assert(index < tokenList->tokenCount);

    // Shift everything in the list, after the one to delete, back by
    // one element.
    for(i = index + 1; i < tokenList->tokenCount; i++) {
        tokenList->tokens[i - 1] = tokenList->tokens[i];
    }

    tokenList->tokenCount--;
}

void nkppPathTidyPath_clearTokens(
    struct NkppState *state,
    struct NkppPathTidyPathTokens *tokenList)
{
    nkppFree(state, tokenList->tokens);
    tokenList->tokens = NULL;
    tokenList->tokenCount = 0;
}

char *nkppPathTidyPath(
    struct NkppState *state,
    const char *path)
{
    char *pathCopy = NULL;
    nkuint32_t srcPathLen = 0;
    char *ret = NULL;
    nkuint32_t i = 0;
    nkuint32_t tokenStart = 0;
    struct NkppPathTidyPathTokens tokens = {0};
    nkbool pathIsAbsoluteUnix = nkfalse;
    nkuint32_t outputLength = 0;
    char *outputPtr = NULL;

    pathCopy = nkppStrdup(state, path);
    if(!pathCopy) {
        goto nkppPathTidyPath_cleanup;
    }

    srcPathLen = nkppStrlen(pathCopy);

    // Zero-length paths become ".".
    if(srcPathLen == 0) {
        ret = nkppStrdup(state, ".");
        goto nkppPathTidyPath_cleanup;
    }

    // For cases of '.', '/', or other single-character relative
    // paths.
    if(srcPathLen == 1) {
        ret = nkppStrdup(state, pathCopy);
        goto nkppPathTidyPath_cleanup;
    }

    // Chop off trailing slashes.
    while(srcPathLen > 1 && nkppPathCharIsSeparator(pathCopy[srcPathLen - 1])) {
        pathCopy[srcPathLen - 1] = 0;
        srcPathLen--;
    }

    tokenStart = 0;
    for(i = 0; i < srcPathLen + 1; i++) {

        if(nkppPathCharIsSeparator(pathCopy[i]) || i == srcPathLen) {

            if(nkppPathCharIsSeparator(pathCopy[i])) {
                pathCopy[i] = 0;
            }

            if(i == tokenStart) {

                if(i == 0) {
                    // We got an "empty" token at the very beginning,
                    // meaning that the first character was a slash.
                    // That means this is an absolute path in *nix.
                    pathIsAbsoluteUnix = nktrue;
                }

            } else {

                // Normal token. Add it.
                if(!nkppPathTidyPath_addToken(
                        state,
                        pathCopy + tokenStart,
                        &tokens))
                {
                    goto nkppPathTidyPath_cleanup;
                }
            }

            tokenStart = i + 1;
        }

    }

    // Now that we have the token list, we can start deleting stuff
    // from it. The easiest thing to start with is getting rid of all
    // the "." elements.
    for(i = 0; i < tokens.tokenCount; i++) {
        if(!nkppStrcmp(tokens.tokens[i], ".")) {
            nkppPathTidyPath_deleteToken(&tokens, i);
            i--;
        }
    }

    // Get rid of '..' and whatever came before it, if it's the first
    // element or has a '..' before it, then leave it.
    for(i = 0; i < tokens.tokenCount; i++) {
        if(!nkppStrcmp(tokens.tokens[i], "..")) {

            // If we're at the very beginning, and this is an absolute
            // path, then we can't go up any further! This path is
            // invalid in a way that we can't fix. Chopping off extra
            // ".."s from the beginning would result in lost
            // information.
            if(i == 0 && pathIsAbsoluteUnix) {
                goto nkppPathTidyPath_cleanup;
            }

            // If we're at the beginning and it's not an absolute
            // path, then we just have to assume it's in a directory
            // relative (up) from the CWD.
            if(i == 0) {
                continue;
            }

            // If the last token was "..", and it's still there, then
            // it must have been preserved by one of the other cases,
            // in which case this just means further up (relatively)
            // from here.
            if(!nkppStrcmp(tokens.tokens[i - 1], "..")) {
                continue;
            }

            // Delete what came before it.
            nkppPathTidyPath_deleteToken(&tokens, i - 1);

            // Delete it (now that the index has changed).
            nkppPathTidyPath_deleteToken(&tokens, i - 1);

            // Back up by *two* indices for the two we deleted.
            i -= 2;
        }
    }

    // Figure out final output length.
    outputLength = pathIsAbsoluteUnix ? 1 : 0;
    for(i = 0; i < tokens.tokenCount; i++) {
        outputLength = outputLength + nkppStrlen(tokens.tokens[i]);
        // Add room for the slash after this.
        if(i != tokens.tokenCount - 1) {
            outputLength++;
        }
    }

    // If there are no path elements at this point, then we're going
    // to return a '.'. So make room for that.
    if(tokens.tokenCount == 0 && !pathIsAbsoluteUnix) {
        outputLength++;
    }

    // Add null terminator room.
    outputLength++;

    // Allocate it.
    ret = nkppMalloc(state, outputLength);
    if(!ret) {
        goto nkppPathTidyPath_cleanup;
    }

    // Write out tokens and leading '/' for full paths, or just '.' if
    // we have no elements.
    outputPtr = ret;
    if(pathIsAbsoluteUnix) {
        outputPtr[0] = '/';
        outputPtr++;
    }
    if(tokens.tokenCount == 0 && !pathIsAbsoluteUnix) {
        outputPtr[0] = '.';
        outputPtr++;
    }
    for(i = 0; i < tokens.tokenCount; i++) {
        nkuint32_t tokenLen = nkppStrlen(tokens.tokens[i]);
        nkppMemcpy(outputPtr, tokens.tokens[i], tokenLen);
        outputPtr += tokenLen;
        if(i != tokens.tokenCount - 1) {
            outputPtr[0] = '/';
            outputPtr++;
        }
    }
    outputPtr[0] = 0;

nkppPathTidyPath_cleanup:

    if(pathCopy) {
        nkppFree(state, pathCopy);
    }
    nkppPathTidyPath_clearTokens(state, &tokens);

    return ret;
}

char *nkppPathAppend(
    struct NkppState *state,
    const char *path1,
    const char *path2)
{
    nkuint32_t len1 = nkppStrlen(path1);
    nkuint32_t len2 = nkppStrlen(path2);
    nkuint32_t bufferSize = 0;
    nkbool overflow = nkfalse;
    char *tmp = NULL;
    char *ret = NULL;

    // Create a crudely-constructed version of the combined path.
    NK_CHECK_OVERFLOW_UINT_ADD(len1, len2, bufferSize, overflow);
    NK_CHECK_OVERFLOW_UINT_ADD(bufferSize, 2, bufferSize, overflow);
    if(overflow) {
        return NULL;
    }

    tmp = nkppMalloc(state, bufferSize);
    if(!tmp) {
        return NULL;
    }

    nkppMemcpy(tmp, path1, len1);
    tmp[len1] = '/';
    nkppMemcpy(tmp + len1 + 1, path2, len2);
    tmp[len1 + 1 + len2] = 0;

    // Now just tidy that up.
    ret = nkppPathTidyPath(state, tmp);

    nkppFree(state, tmp);
    return ret;
}

nkbool nkppPathIsAbsolute(
    const char *path)
{
    // Zero-length paths.
    if(!path[0]) {
        return nkfalse;
    }

    // Unix-style absolute paths.
    if(nkppPathCharIsSeparator(path[0])) {
        return nktrue;
    }

    // Windows-style (drive letter) absolute paths.
    if((path[0] >= 'a' && path[0] <= 'z') ||
        (path[0] >= 'A' && path[0] <= 'Z'))
    {
        if(path[1] == ':') {
            return nktrue;
        }
    }

    return nkfalse;
}

#if NK_PP_ENABLETESTS

nkbool nkppPathTest_checkString(struct NkppState *state, char *testOutput, const char *reference)
{
    nkbool ret = !nkppStrcmp(testOutput, reference);
    return ret;
}

#define NK_PP_PATHTEST_CHECK(x, y)                                  \
    do {                                                            \
        char *testVal;                                              \
        printf("%-80s : ", #x " == " #y);                           \
        testVal = (x);                                              \
        if(testVal) {                                               \
            if(!nkppPathTest_checkString(state, testVal, (y))) {    \
                printf("%s\n", NK_PPTEST_FAIL);                     \
                ret = nkfalse;                                      \
            } else {                                                \
                printf("%s\n", NK_PPTEST_PASS);                     \
            }                                                       \
        } else {                                                    \
            printf("%s\n", NK_PPTEST_NULL);                         \
        }                                                           \
        nkppFree(state, testVal);                                   \
    } while(0)

#undef NK_PP_PATHTEST_CHECK

#define NK_PP_PATHTEST_CHECK(x, y)              \
    do {                                        \
        NK_PPTEST_CHECK(!nkppStrcmp((x), (y))); \
        nkppFree(state, (x));                   \
    } while(0);

nkbool nkppTest_pathTest(void)
{
    struct NkppState *state = nkppStateCreate(NULL, NULL);
    nkbool ret = nktrue;

    NK_PPTEST_SECTION("nkppTest_pathTest()");

    NK_PPTEST_CHECK(state);

    if(!state) {
        return nkfalse;
    }

    NK_PPTEST_SECTION("nkppPathDirname()");

    NK_PP_PATHTEST_CHECK(nkppPathDirname(state, "/foo"),                    "/");
    NK_PP_PATHTEST_CHECK(nkppPathDirname(state, "foo"),                     ".");
    NK_PP_PATHTEST_CHECK(nkppPathDirname(state, "foo/"),                    ".");
    NK_PP_PATHTEST_CHECK(nkppPathDirname(state, "/foo/"),                   "/");
    NK_PP_PATHTEST_CHECK(nkppPathDirname(state, "directoryname/foo/bar"),   "directoryname/foo");
    NK_PP_PATHTEST_CHECK(nkppPathDirname(state, "directoryname/foo/bar/"),  "directoryname/foo");
    NK_PP_PATHTEST_CHECK(nkppPathDirname(state, "/f/b"),                    "/f");

    // This test case is weird, because it tests some not-well-formed
    // input and spits back some not-well-formed output. We can change
    // this test case later if we want to spit back better formed
    // results.
    //
    // We still want a test case here for the usual memory
    // leak/crash/uninitialized mememy/whatever testing.
    NK_PP_PATHTEST_CHECK(nkppPathDirname(state,   "/////f/b/////"),               "/////f");

    NK_PPTEST_SECTION("nkppPathBasename()");

    NK_PP_PATHTEST_CHECK(nkppPathBasename(state,  "/foo"),                        "foo");
    NK_PP_PATHTEST_CHECK(nkppPathBasename(state,  "foo"),                         "foo");
    NK_PP_PATHTEST_CHECK(nkppPathBasename(state,  "foo/"),                        "foo");
    NK_PP_PATHTEST_CHECK(nkppPathBasename(state,  "/foo/"),                       "foo");
    NK_PP_PATHTEST_CHECK(nkppPathBasename(state,  "directoryname/foo/bar"),       "bar");
    NK_PP_PATHTEST_CHECK(nkppPathBasename(state,  "directoryname/foo/bar/"),      "bar");
    NK_PP_PATHTEST_CHECK(nkppPathBasename(state,  "/f/b"),                        "b");
    NK_PP_PATHTEST_CHECK(nkppPathBasename(state,  "/f/b2"),                       "b2");
    NK_PP_PATHTEST_CHECK(nkppPathBasename(state,  "/////f/b/////"),               "b");

    NK_PPTEST_SECTION("nkppPathTidyPath()");

    NK_PP_PATHTEST_CHECK(nkppPathTidyPath(state,  "/foo"),                        "/foo");
    NK_PP_PATHTEST_CHECK(nkppPathTidyPath(state,  "foo"),                         "foo");
    NK_PP_PATHTEST_CHECK(nkppPathTidyPath(state,  "foo/"),                        "foo");
    NK_PP_PATHTEST_CHECK(nkppPathTidyPath(state,  "/foo/"),                       "/foo");
    NK_PP_PATHTEST_CHECK(nkppPathTidyPath(state,  "directoryname/foo/bar"),       "directoryname/foo/bar");
    NK_PP_PATHTEST_CHECK(nkppPathTidyPath(state,  "directoryname/foo/bar/"),      "directoryname/foo/bar");
    NK_PP_PATHTEST_CHECK(nkppPathTidyPath(state,  "/f/b"),                        "/f/b");
    NK_PP_PATHTEST_CHECK(nkppPathTidyPath(state,  "/f/b2"),                       "/f/b2");
    NK_PP_PATHTEST_CHECK(nkppPathTidyPath(state,  "/////f/b/////"),               "/f/b");
    NK_PP_PATHTEST_CHECK(nkppPathTidyPath(state,  "/f/b/././././../../a/b/c/.."), "/a/b");
    NK_PP_PATHTEST_CHECK(nkppPathTidyPath(state,  "////"),                        "/");
    NK_PP_PATHTEST_CHECK(nkppPathTidyPath(state,  ""),                            ".");

    NK_PPTEST_SECTION("nkppPathAppend()");

    NK_PP_PATHTEST_CHECK(nkppPathAppend(state, "../", "/.."), "../..");
    NK_PP_PATHTEST_CHECK(nkppPathAppend(state, "a/b//c/d", "/../"), "a/b/c");
    NK_PP_PATHTEST_CHECK(nkppPathAppend(state, "a/b//c/d", "/../../../../"), ".");
    NK_PP_PATHTEST_CHECK(nkppPathAppend(state, "/a/b//c/d", "/../../../../"), "/");
    NK_PP_PATHTEST_CHECK(nkppPathAppend(state, "a/b/./././././././c/d", "/../../../../"), ".");

    NK_PPTEST_SECTION("nkppPathIsAbsolute()");

    NK_PPTEST_CHECK(nkppPathIsAbsolute("c:/")        == nktrue);
    NK_PPTEST_CHECK(nkppPathIsAbsolute("./c:/")      == nktrue);
    NK_PPTEST_CHECK(nkppPathIsAbsolute(".")          == nkfalse);
    NK_PPTEST_CHECK(nkppPathIsAbsolute("/")          == nktrue);
    NK_PPTEST_CHECK(nkppPathIsAbsolute("\\whatever") == nktrue);

    nkppStateDestroy(state);

    return ret;
}

#endif // NK_PP_ENABLETESTS

