#ifndef NK_PPSTRING_H
#define NK_PPSTRING_H

#include "ppcommon.h"

// ----------------------------------------------------------------------
// String transformation

/// Return a new copy of a string with all the special characters
/// escaped with backslashes.
char *nkppEscapeString(struct NkppState *state, const char *src);

/// Return a new copy of a string with all the escaped newlines
/// removed.
char *nkppDeleteBackslashNewlines(struct NkppState *state, const char *str);

/// Return a new copy of a string with comments removed, and any
/// whitespace at the start and end trimmed off.
char *nkppStripCommentsAndTrim(struct NkppState *state, const char *in);

// ----------------------------------------------------------------------
// General string stuff

char *nkppStrdup(struct NkppState *state, const char *s);

void nkppMemcpy(void *dst, const void *src, nkuint32_t len);

int nkppStrcmp(const char *a, const char *b);

nkuint32_t nkppStrlen(const char *str);

/// Slightly different from the "real" strtol. Base is determined
/// automatically (0x10 == 16, etc).
nkbool nkppStrtol(const char *str, nkuint32_t *out);

// ----------------------------------------------------------------------
// Character classification

nkbool nkppIsWhitespace(char c);
nkbool nkppIsValidIdentifierCharacter(char c, nkbool isFirstCharacter);
nkbool nkppIsDigit(char c);
nkbool nkppIsDigitHex(char c);

#endif // NK_PPSTRING_H

