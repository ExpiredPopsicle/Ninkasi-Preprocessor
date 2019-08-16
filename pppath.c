#include "ppcommon.h"

// Returns NK_UINT_MAX if there is no slash.
nkuint32_t nkppPathFindLastSlash(
    const char *path)
{
    nkuint32_t i = nkppStrlen(path);

    // Skip trailing '/'.
    if(i > 1) {
        i--;
        if(path[i] == '/') {
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
    if(retLen > 1) {
        if(ret[retLen - 1] == '/') {
            ret[retLen - 1] = 0;
        }
    }

    return ret;
}

char *nkppPathTidyPath(
    struct NkppState *state,
    const char *path)
{
    return NULL;
}

char *nkppPathAppend(
    struct NkppState *state,
    const char *path1,
    const char *path2)
{
    return NULL;
}

