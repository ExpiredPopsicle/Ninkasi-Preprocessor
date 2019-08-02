#ifndef NK_PPSTATE_H
#define NK_PPSTATE_H

#include "ppcommon.h"

// ----------------------------------------------------------------------
// Types

struct PreprocessorMacro;

struct PreprocessorError
{
    char *filename;
    nkuint32_t lineNumber;
    char *text;
    struct PreprocessorError *next;
};

struct PreprocessorErrorState
{
    struct PreprocessorError *firstError;
    struct PreprocessorError *lastError;
};

typedef void *(*NkppMallocWrapper)(void *userData, nkuint32_t size);
typedef void (*NkppFreeWrapper)(void *userData, void *ptr);

struct PreprocessorMemoryCallbacks
{
    NkppFreeWrapper freeWrapper;
    NkppMallocWrapper mallocWrapper;
    void *userData;
};

// Main preprocessor state object.
struct PreprocessorState
{
    // Input buffer and position.
    const char *str;
    nkuint32_t index;

    // Output buffer (position always at end).
    char *output;

    // Line number in the source file.
    nkuint32_t lineNumber;

    // Line number in the output after being fffected by #line
    // directives. This should match the input lineNumber. When it
    // gets out of sync it means we need to add a new #line directive
    // and then set it back.
    nkuint32_t outputLineNumber;

    // Macro list.
    struct PreprocessorMacro *macros;

    // Whether we should be writing position markers or not. This gets
    // turned off when we're in the middle of outputting a macro, and
    // switched back on when we get back to the normal source file.
    nkbool writePositionMarkers;

    // nktrue if we should update line markers if they're out of sync
    // (input vs output) once we get back to the main loop. nkfalse
    // otherwise.
    nkbool updateMarkers;

    // Current source filename.
    char *filename;

    // Non-owning pointer to the error state. This will be shared
    // between many PreprocessorStates. If this is NULL, then errors
    // will be directed to stderr.
    struct PreprocessorErrorState *errorState;

    nkuint32_t nestedPassedIfs;
    nkuint32_t nestedFailedIfs;

    struct PreprocessorMemoryCallbacks *memoryCallbacks;
};

// ----------------------------------------------------------------------
// General state object

struct PreprocessorState *createPreprocessorState(
    struct PreprocessorErrorState *errorState,
    struct PreprocessorMemoryCallbacks *memoryCallbacks);

void destroyPreprocessorState(struct PreprocessorState *state);

struct PreprocessorState *preprocessorStateClone(
    const struct PreprocessorState *state);

/// Set the current file name. Used for error tracking.
nkbool preprocessorStateSetFilename(
    struct PreprocessorState *state,
    const char *filename);

// ----------------------------------------------------------------------
// Macros

void preprocessorStateAddMacro(
    struct PreprocessorState *state,
    struct PreprocessorMacro *macro);
struct PreprocessorMacro *preprocessorStateFindMacro(
    struct PreprocessorState *state,
    const char *identifier);
nkbool preprocessorStateDeleteMacro(
    struct PreprocessorState *state,
    const char *identifier);

// ----------------------------------------------------------------------
// Read/write functions

/// Append a string to the output.
nkbool appendString(struct PreprocessorState *state, const char *str);

/// Append a character to the output. Output line number tracking
/// happens here. Use this (or a function that calls it, like
/// appendString()) to output instead of maually updating the buffer.
nkbool appendChar(struct PreprocessorState *state, char c);

/// Advance the read pointer. Optionally outputting the character.
/// Input line number tracking happens here, so always use this to
/// advance the read pointer instead of messing with the index
/// yourself.
nkbool skipChar(struct PreprocessorState *state, nkbool output);

/// Skip past whitespace and comments, optionally sending them to the
/// output. This is to preserve formatting and comments in the output.
nkbool skipWhitespaceAndComments(
    struct PreprocessorState *state,
    nkbool output,
    nkbool stopAtNewline);

/// Flag the preprocessor state as needing an updated file/line marker
/// in the output. This should be used immediately after outputting
/// something that would disrupt the syncronization between the input
/// and output line number tracking. Something like a multi-line
/// macro, or an include directive.
///
/// The file/line directive doesn't automatically get inserted without
/// this flag being set, to prevent us from outputting it in the
/// middle of one of those multiline macros.
void preprocessorStateFlagFileLineMarkersForUpdate(
    struct PreprocessorState *state);

/// Clear the output buffer entirely.
void preprocessorStateClearOutput(struct PreprocessorState *state);

// ----------------------------------------------------------------------
// Tokenization

struct PreprocessorToken *getNextToken(
    struct PreprocessorState *state,
    nkbool outputWhitespace);
char *readIdentifier(struct PreprocessorState *state);
char *readQuotedString(struct PreprocessorState *state);
char *readInteger(struct PreprocessorState *state);
char *readMacroArgument(struct PreprocessorState *state);

// ----------------------------------------------------------------------
// Errors

/// Record an error. The error will be attributed to the current line
/// number and file name.
void preprocessorStateAddError(
    struct PreprocessorState *state,
    const char *errorMessage);

// ----------------------------------------------------------------------
// ifdef/ifndef/if/else/endif handling

nkbool preprocessorStatePushIfResult(
    struct PreprocessorState *state,
    nkbool ifResult);

nkbool preprocessorStatePopIfResult(
    struct PreprocessorState *state);

/// For handling the topmost "else" statement.
nkbool preprocessorStateFlipIfResult(
    struct PreprocessorState *state);

// ----------------------------------------------------------------------
// Allocations within the parser

void *nkppMalloc(struct PreprocessorState *state, nkuint32_t size);

void nkppFree(struct PreprocessorState *state, void *ptr);

#endif // NK_PPSTATE_H
