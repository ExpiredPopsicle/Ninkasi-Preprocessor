#ifndef NK_PPX_H
#define NK_PPX_H

#include "pptypes.h"

// ----------------------------------------------------------------------
// Types

struct NkppErrorState;
struct NkppState;

typedef void *(*NkppMallocWrapper)(
    void *userData,
    nkuint32_t size);

typedef void (*NkppFreeWrapper)(
    void *userData,
    void *ptr);

typedef char *(*NkppLoadFileCallback)(
    struct NkppState *state,
    void *userData,
    const char *filename,
    nkbool systemInclude);

struct NkppMemoryCallbacks
{
    /// This may return NULL to indicate an allocation failure.
    /// Otherwise it should act like malloc().
    ///
    /// If this is not specified, nkppDefaultMallocWrapper() will be
    /// used.
    NkppMallocWrapper mallocWrapper;

    /// This function must behave like free() for anything allocated
    /// using mallocWrapper.
    ///
    /// If this is not specified, nkppDefaultFreeWrapper() will be
    /// used.
    NkppFreeWrapper freeWrapper;

    /// This callback is called when we need to load a file from an
    /// #include. Returns NULL to indicate failure. Otherwise, returns
    /// a null-terminated string with the contents of the included
    /// file.
    ///
    /// Access control must be implemented inside this function to
    /// prevent unauthorized reading.
    ///
    /// Memory returned by this must be allocated with nkppMalloc(),
    /// so that it may be freed by the preprocessor.
    ///
    /// If this is left as NULL, then a default insecure callback will
    /// be used in its place.
    NkppLoadFileCallback loadFileCallback;

    /// Userdata used by all callbacks.
    void *userData;
};

// ----------------------------------------------------------------------
// NkppState manipulation

struct NkppState *nkppStateCreate(
    struct NkppMemoryCallbacks *memoryCallbacks);

void nkppStateDestroy(
    struct NkppState *state);

/// Execute the preprocessor on some text.
nkbool nkppStateExecute(
    struct NkppState *state,
    const char *str,
    const char *filename);

/// Execute a #define directive immediately.
nkbool nkppStateAddDefine(
    struct NkppState *state,
    const char *line);

/// Get the number of errors in the code that was processed.
nkuint32_t nkppStateGetErrorCount(
    const struct NkppState *state);

/// Get an error message. This function fills in pointers to the file
/// name and error message. These strings are owned by the
/// NkppErrorState on the NkppState itself and will become invalid
/// once the state is destroyed. Returns nkfalse if the error cannot
/// be filled in.
nkbool nkppStateGetError(
    const struct NkppState *state,
    nkuint32_t errorIndex,
    const char **outFilename,
    nkuint32_t *outLineNumber,
    const char **outErrorMessage);

nkuint32_t nkppStateHasError(
    const struct NkppState *state);

/// Add an include path. These are paths that will be attempted first
/// when the script uses the brackets ("<>") instead of quotes for
/// include directives. Note that depending on your implementation of
/// loadFileCallback, this may not have to correspond to a real path.
nkbool nkppStateAddIncludePath(
    struct NkppState *state,
    const char *path);

/// Use this to retrieve the final output.
const char *nkppStateGetOutput(
    const struct NkppState *state);

// ----------------------------------------------------------------------
// Memory/file functions
//
// Use these to allocate and free memory associated with a given
// NkppState.

/// This is the malloc() replacement used to allocate memory
/// associated with the preprocessor. Memory allocated with this must
/// be freed with nkppFree().
void *nkppMalloc(
    struct NkppState *state,
    nkuint32_t size);

/// This is the corresponding free() replacement to nkppMalloc().
void nkppFree(
    struct NkppState *state,
    void *ptr);

/// This is the realloc() replacement that goes with nkppMalloc() and
/// nkppFree().
void *nkppRealloc(
    struct NkppState *state,
    void *ptr,
    nkuint32_t size);

/// This is the default file loader callack. If loadFileCallback is
/// NULL, this will be used.
char *nkppSimpleLoadFile(
    struct NkppState *state,
    const char *filename);

#endif // NK_PPX_H
