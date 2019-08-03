#ifndef NK_PPSTRING_H
#define NK_PPSTRING_H

#include "ppcommon.h"

/// Return a new copy of a string with all the special characters
/// escaped with backslashes.
char *nkppEscapeString(struct NkppState *state, const char *src);

/// Return a new copy of a string with all the escaped newlines
/// removed.
char *nkppDeleteBackslashNewlines(struct NkppState *state, const char *str);

/// Return a new copy of a string with comments removed, and any
/// whitespace at the start and end trimmed off.
char *nkppStripCommentsAndTrim(struct NkppState *state, const char *in);

#endif // NK_PPSTRING_H

