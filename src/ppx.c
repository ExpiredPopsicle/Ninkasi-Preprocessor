// ----------------------------------------------------------------------
//
//   Ninkasi Preprocessor 0.1
//
//   By Kiri "ExpiredPopsicle" Jolly
//     https://expiredpopsicle.com
//     https://intoxicoding.com
//     expiredpopsicle@gmail.com
//
// ----------------------------------------------------------------------
//
//   Copyright (c) 2019 Kiri Jolly
//
//   Permission is hereby granted, free of charge, to any person
//   obtaining a copy of this software and associated documentation files
//   (the "Software"), to deal in the Software without restriction,
//   including without limitation the rights to use, copy, modify, merge,
//   publish, distribute, sublicense, and/or sell copies of the Software,
//   and to permit persons to whom the Software is furnished to do so,
//   subject to the following conditions:
//
//   The above copyright notice and this permission notice shall be
//   included in all copies or substantial portions of the Software.
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
//   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
//   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//   SOFTWARE.
//
// -------------------------- END HEADER -------------------------------------

#include "ppcommon.h"

struct NkppState *nkppStateCreate(
    struct NkppMemoryCallbacks *memoryCallbacks)
{
    NkppMallocWrapper mallocWrapper =
        memoryCallbacks && memoryCallbacks->mallocWrapper ?
        memoryCallbacks->mallocWrapper : nkppDefaultMallocWrapper;
    NkppFreeWrapper freeWrapper =
        memoryCallbacks && memoryCallbacks->freeWrapper ?
        memoryCallbacks->freeWrapper : nkppDefaultFreeWrapper;
    void *userData =
        memoryCallbacks ? memoryCallbacks->userData : NULL;

    struct NkppErrorState *errorState = NULL;
    struct NkppState *state = NULL;

    errorState =
        mallocWrapper(userData, sizeof(struct NkppErrorState));
    if(!errorState) {
        return NULL;
    }

    nkppErrorStateInit(errorState);

    state = nkppStateCreate_internal(errorState, memoryCallbacks);
    if(!state) {
        freeWrapper(userData, errorState);
        return NULL;
    }

    // This is the outermost instance of the NkppState, and it's the
    // only one that's responsible for writing position markers.
    state->writePositionMarkers = nktrue;

    return state;
}

void nkppStateDestroy(
    struct NkppState *state)
{
    NkppFreeWrapper freeWrapper = NULL;
    void *userData = NULL;

    assert(state);

    freeWrapper =
        state->memoryCallbacks && state->memoryCallbacks->freeWrapper ?
        state->memoryCallbacks->freeWrapper : nkppDefaultFreeWrapper;
    userData =
        state->memoryCallbacks ? state->memoryCallbacks->userData : NULL;

    nkppErrorStateClear(state, state->errorState);

    freeWrapper(userData, state->errorState);

    nkppStateDestroy_internal(state);
}

nkbool nkppStateAddBuiltinDefines(
    struct NkppState *state)
{
    // The time/date defines in here seem like the enemy of
    // reproducible builds, and possibly even an anti-feature, but
    // they're included for C89 completeness.

    time_t currentTime;
    struct tm *currentTimeLocal;
    char tmp[256];
    const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    };

    time(&currentTime);
    currentTimeLocal = localtime(&currentTime);

    sprintf(tmp, "__TIME__ \"%.2d:%.2d:%.2d\"",
        currentTimeLocal->tm_hour,
        currentTimeLocal->tm_min,
        currentTimeLocal->tm_sec);
    if(!nkppStateAddDefine(state, tmp)) {
        return nkfalse;
    }

    sprintf(tmp, "__DATE__ \"%s %2d %4d\"",
        months[currentTimeLocal->tm_mon],
        currentTimeLocal->tm_mday,
        1900 + currentTimeLocal->tm_year);
    if(!nkppStateAddDefine(state, tmp)) {
        return nkfalse;
    }

    return nktrue;
}

nkbool nkppStateExecute(
    struct NkppState *state,
    const char *str,
    const char *filename)
{
    assert(state);

    if(!nkppStateAddBuiltinDefines(state)) {
        return nkfalse;
    }

    if(filename) {
        if(!nkppStateSetFilename(state, filename)) {
            return nkfalse;
        }
    }

    return nkppStateExecute_internal(state, str);
}

nkuint32_t nkppStateGetErrorCount(
    const struct NkppState *state)
{
    nkuint32_t count = 0;
    struct NkppError *currentError;

    assert(state);
    assert(state->errorState);

    currentError = state->errorState->firstError;
    while(currentError) {
        count++;
        currentError = currentError->next;
    }

    if(state->errorState->allocationFailure) {
        count++;
    }

    return count;
}

nkbool nkppStateHasError(
    const struct NkppState *state)
{
    assert(state);
    assert(state->errorState);

    if(state->errorState->allocationFailure) {
        return nktrue;
    }

    if(state->errorState->firstError) {
        return nktrue;
    }

    return nkfalse;
}

nkbool nkppStateAddDefine(
    struct NkppState *state,
    const char *line)
{
    return nkppDirective_define(state, line);
}

nkbool nkppStateAddIncludePath(
    struct NkppState *state,
    const char *path)
{
    nkuint32_t newPathCount;
    nkbool overflow = nkfalse;
    char **newPathList;
    char *copiedPath;

    NK_CHECK_OVERFLOW_UINT_ADD(
        state->includePathCount, 1,
        newPathCount, overflow);
    if(overflow) {
        return nkfalse;
    }

    copiedPath = nkppStrdup(state, path);
    if(!copiedPath) {
        return nkfalse;
    }

    newPathList = nkppRealloc(
        state, state->includePaths,
        sizeof(char*) * newPathCount);
    if(!newPathList) {
        nkppFree(state, copiedPath);
        return nkfalse;
    }
    state->includePaths = newPathList;

    state->includePaths[state->includePathCount] = copiedPath;
    state->includePathCount = newPathCount;

    return nktrue;
}

nkbool nkppStateGetError(
    const struct NkppState *state,
    nkuint32_t errorIndex,
    const char **outFilename,
    nkuint32_t *outLineNumber,
    const char **outErrorMessage)
{
    struct NkppError *currentError;

    if(!state->errorState) {
        return nkfalse;
    }

    currentError = state->errorState->firstError;
    while(currentError && errorIndex) {
        currentError = currentError->next;
        errorIndex--;
    }

    if(!currentError) {
        if(errorIndex == 0 && state->errorState->allocationFailure) {
            *outFilename = "<unknown>";
            *outErrorMessage = "Allocation failure. All other error messages may be the result of incorrect behavior.";
            *outLineNumber = 0;
            return nktrue;
        }
        return nkfalse;
    }

    *outFilename = currentError->filename;
    *outErrorMessage = currentError->text;
    *outLineNumber = currentError->lineNumber;

    return nktrue;
}

const char *nkppStateGetOutput(
    const struct NkppState *state)
{
    return state->output;
}

char *nkppSimpleLoadFile(
    struct NkppState *state,
    const char *filename)
{
    return nkppDefaultLoadFileCallback(state, NULL, filename, nkfalse);
}

