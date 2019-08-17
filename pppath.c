#include "ppcommon.h"

// Returns NK_UINT_MAX if there is no slash.
nkuint32_t nkppPathFindLastSlash(
    const char *path)
{
    nkuint32_t i = nkppStrlen(path);

    // Skip trailing '/'s.
    if(i > 1) {
        i--;
        while(i && path[i] == '/') {
            i--;
        }
    }

    // Seek backwards from the beginning.
    while(i != NK_UINT_MAX) {
        if(path[i] == '/') {
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
    while(retLen > 1 && ret[retLen - 1] == '/') {
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

    // FIXME: Check overflow.
    tokenList->tokenCount++;

    newTokenList = nkppRealloc(
        state,
        tokenList->tokens,
        sizeof(char*) * tokenList->tokenCount);
    if(!newTokenList) {
        return nkfalse;
    }

    tokenList->tokens = newTokenList;

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
    while(srcPathLen > 1 && pathCopy[srcPathLen - 1] == '/') {
        pathCopy[srcPathLen - 1] = 0;
        srcPathLen--;
    }

    tokenStart = 0;
    for(i = 0; i < srcPathLen + 1; i++) {

        if(pathCopy[i] == '/' || i == srcPathLen) {

            if(pathCopy[i] == '/') {
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
