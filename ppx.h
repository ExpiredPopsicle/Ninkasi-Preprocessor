#ifndef NK_PPX_H
#define NK_PPX_H

#include "pptypes.h"

struct NkppErrorState;

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
    NkppLoadFileCallback loadFileCallback;

    /// Userdata used by all callbacks.
    void *userData;
};

struct NkppState *nkppStateCreate(
    struct NkppMemoryCallbacks *memoryCallbacks);

void nkppStateDestroy(
    struct NkppState *state);

nkbool nkppStateExecute(
    struct NkppState *state,
    const char *str,
    const char *filename);

/// Execute a #define directive immediately.
nkbool nkppStateAddDefine(
    struct NkppState *state,
    const char *line);

nkuint32_t nkppStateGetErrorCount(
    const struct NkppState *state);

nkuint32_t nkppStateHasError(
    const struct NkppState *state);

#endif // NK_PPX_H
