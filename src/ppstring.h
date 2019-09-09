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

#ifndef NK_PPSTRING_H
#define NK_PPSTRING_H

#include "ppcommon.h"

// ----------------------------------------------------------------------
// String transformation

/// Return a new copy of a string with all the special characters
/// escaped with backslashes.
char *nkppEscapeString(struct NkppState *state, const char *src);

char *nkppRemoveQuotes(
    struct NkppState *state,
    const char *src,
    nkbool careAboutBackslashes);

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

nkuint32_t nkppParseDigit(char c);

/// Slightly different from the "real" strtol. Base is determined
/// automatically (0x10 == 16, etc). Also, this is unsigned-only
/// (parsing of negation operator happens separately).
nkbool nkppStrtoui(const char *str, nkuint32_t *out);

// ----------------------------------------------------------------------
// Character classification

nkbool nkppIsWhitespace(char c);
nkbool nkppIsValidIdentifierCharacter(char c, nkbool isFirstCharacter);
nkbool nkppIsDigit(char c);
nkbool nkppIsDigitHex(char c);

#endif // NK_PPSTRING_H

