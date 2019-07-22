#ifndef NK_PPSTATE_H
#define NK_PPSTATE_H

#include "ppcommon.h"

// ----------------------------------------------------------------------
// Types

struct PreprocessorMacro;

// Main preprocessor state object.
struct PreprocessorState
{
    // TODO: Source filename.

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

    // TODO: Error state.

    nkbool updateMarkers;
};

// ----------------------------------------------------------------------
// State object

struct PreprocessorState *createPreprocessorState(void);
void destroyPreprocessorState(struct PreprocessorState *state);
struct PreprocessorState *preprocessorStateClone(
    const struct PreprocessorState *state);

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

void appendString(struct PreprocessorState *state, const char *str);
void appendChar(struct PreprocessorState *state, char c);
void skipChar(struct PreprocessorState *state, nkbool output);
void skipWhitespaceAndComments(
    struct PreprocessorState *state,
    nkbool output,
    nkbool stopAtNewline);
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

void preprocessorStateAddError(
    struct PreprocessorState *state,
    const char *errorMessage);

#endif // NK_PPSTATE_H
