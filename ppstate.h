#ifndef NK_PPSTATE_H
#define NK_PPSTATE_H

#include "pptypes.h"

// ----------------------------------------------------------------------
// Types

struct NkppMacro;
struct NkppError;
struct NkppErrorState;
struct NkppMemoryCallbacks;

struct NkppStateConditional
{
    // Save us time looking all the way through the chain for a
    // failure. Cache whatever the parent result was.
    nkbool parentPassed;

    // Whether this specific conditional has passed. This is the first
    // "if", "ifndef", or "ifdef".
    nkbool passedOriginalConditional;

    // This is whether we have currently passed a test. This could be
    // from an "else", or from an "elif".
    nkbool passed;

    // This will be true if we've used our "else" block, whether we've
    // taken that branch or not.
    //
    // If there are multiple "else" directives at this level, that is
    // an error.
    //
    // If there is an "elif" directive after an "else" directive, that
    // is an error.
    nkbool seenElseAlready;

    // This will be true if we've taken the branch on an "elif", seen
    // our "else" branch, or if the first condition passed. All of
    // these will rule out taking the branch of any more "elif"
    // directives.
    nkbool passedElifAlready;

    struct NkppStateConditional *next;
};

// Main preprocessor state object.
struct NkppState
{
    // Input buffer and position.
    const char *str;
    nkuint32_t index;

    // Output buffer (position always at end).
    char *output;
    nkuint32_t outputCapacity;
    nkuint32_t outputLength;

    // Line number in the source file.
    nkuint32_t lineNumber;

    // Line number in the output after being fffected by #line
    // directives. This should match the input lineNumber. When it
    // gets out of sync it means we need to add a new #line directive
    // and then set it back.
    nkuint32_t outputLineNumber;

    // Macro list.
    struct NkppMacro *macros;

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
    // between many NkppStates. If this is NULL, then errors
    // will be directed to stderr.
    struct NkppErrorState *errorState;

    struct NkppStateConditional *conditionalStack;

    struct NkppMemoryCallbacks *memoryCallbacks;

    nkuint32_t recursionLevel;

    // Concatenation ("##") is only enabled in certain cases. We don't
    // do it outside of #defines.
    nkbool concatenationEnabled;

    // Number of tokens we've seen on the current line.
    nkuint32_t tokensOnThisLine;

    // We need to know if we're preprocessing a block for an "if"
    // expression. If we are, then we need to skip macros inside of
    // "defined()" expressions.
    nkbool preprocessingIfExpression;

    char **includePaths;
    nkuint32_t includePathCount;
};

// ----------------------------------------------------------------------
// General state object

struct NkppState *nkppStateCreate_internal(
    struct NkppErrorState *errorState,
    struct NkppMemoryCallbacks *memoryCallbacks);

void nkppStateDestroy_internal(struct NkppState *state);

struct NkppState *nkppStateClone(
    struct NkppState *state,
    nkbool copyOutput);

/// Set the current file name. Used for error tracking.
nkbool nkppStateSetFilename(
    struct NkppState *state,
    const char *filename);

// ----------------------------------------------------------------------
// Macros

// FIXME: Make a more accessible version of this for definitions
// coming in from the hosting application.
void nkppStateAddMacro(
    struct NkppState *state,
    struct NkppMacro *macro);

struct NkppMacro *nkppStateFindMacro(
    struct NkppState *state,
    const char *identifier);

nkbool nkppStateDeleteMacro(
    struct NkppState *state,
    const char *identifier);

// ----------------------------------------------------------------------
// Read/write functions

/// Append a string to the output.
nkbool nkppStateOutputAppendString(struct NkppState *state, const char *str);

/// Append a character to the output. Output line number tracking
/// happens here. Use this (or a function that calls it, like
/// nkppStateOutputAppendString()) to output instead of maually
/// updating the buffer.
nkbool nkppStateOutputAppendChar(struct NkppState *state, char c);

/// Advance the read pointer. Optionally outputting the character.
/// Input line number tracking happens here, so always use this to
/// advance the read pointer instead of messing with the index
/// yourself.
nkbool nkppStateInputSkipChar(struct NkppState *state, nkbool output);

/// Skip past whitespace and comments, optionally sending them to the
/// output. This is to preserve formatting and comments in the output.
nkbool nkppStateInputSkipWhitespaceAndComments(
    struct NkppState *state,
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
void nkppStateFlagFileLineMarkersForUpdate(
    struct NkppState *state);

/// Clear the output buffer entirely.
void nkppStateOutputClear(struct NkppState *state);

// ----------------------------------------------------------------------
// Tokenization

struct NkppToken *nkppStateInputGetNextToken(
    struct NkppState *state,
    nkbool outputWhitespace);

char *nkppStateInputReadIdentifier(struct NkppState *state);
char *nkppStateInputReadQuotedString(struct NkppState *state);
char *nkppStateInputReadInteger(struct NkppState *state);

/// Read an argument on the *invocation* of a macro.
char *nkppStateInputReadMacroArgument(struct NkppState *state);

char *nkppStateInputReadRestOfLine(
    struct NkppState *state,
    nkuint32_t *actualLineCount);

// ----------------------------------------------------------------------
// Errors

/// Record an error. The error will be attributed to the current line
/// number and file name.
void nkppStateAddError(
    struct NkppState *state,
    const char *errorMessage);

// ----------------------------------------------------------------------
// ifdef/ifndef/if/else/endif handling

nkbool nkppStatePushIfResult(
    struct NkppState *state,
    nkbool ifResult);

nkbool nkppStatePopIfResult(
    struct NkppState *state);

/// For handling the topmost "else" statement. This just inverts the
/// result of the "if" on the top of the stack.
nkbool nkppStateFlipIfResultForElse(
    struct NkppState *state);

nkbool nkppStateFlipIfResultForElif(
    struct NkppState *state,
    nkbool result);

/// Check to see if we should be emitting stuff and acting on
/// directives right now.
nkbool nkppStateConditionalOutputPassed(struct NkppState *state);

// ----------------------------------------------------------------------
// Main entrypoint

nkbool nkppStateExecute_internal(
    struct NkppState *state,
    const char *str);

#endif // NK_PPSTATE_H
