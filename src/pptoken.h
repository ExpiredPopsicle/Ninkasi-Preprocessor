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

#ifndef NK_PPTOKEN_H
#define NK_PPTOKEN_H

#include "ppcommon.h"

enum NkppTokenType
{
    NK_PPTOKEN_IDENTIFIER,
    NK_PPTOKEN_QUOTEDSTRING,  // str is string with escape characters unconverted and quotes intact!
    NK_PPTOKEN_BRACKETSTRING,
    NK_PPTOKEN_HASH,          // "#"
    NK_PPTOKEN_DOUBLEHASH,    // "##"
    NK_PPTOKEN_COMMA,
    NK_PPTOKEN_NUMBER,
    NK_PPTOKEN_OPENPAREN,
    NK_PPTOKEN_CLOSEPAREN,
    NK_PPTOKEN_LESSTHAN,
    NK_PPTOKEN_GREATERTHAN,
    NK_PPTOKEN_COMPARISONEQUALS,

    NK_PPTOKEN_MINUS,
    NK_PPTOKEN_TILDE,
    NK_PPTOKEN_EXCLAMATION,
    NK_PPTOKEN_ASTERISK,
    NK_PPTOKEN_SLASH,
    NK_PPTOKEN_PERCENT,
    NK_PPTOKEN_PLUS,
    NK_PPTOKEN_LEFTSHIFT,
    NK_PPTOKEN_RIGHTSHIFT,
    NK_PPTOKEN_BINARYAND,
    NK_PPTOKEN_BINARYOR,
    NK_PPTOKEN_BINARYXOR,
    NK_PPTOKEN_LOGICALOR,
    NK_PPTOKEN_LOGICALAND,
    NK_PPTOKEN_NOTEQUAL,
    NK_PPTOKEN_GREATERTHANOREQUALS,
    NK_PPTOKEN_LESSTHANOREQUALS,
    NK_PPTOKEN_QUESTIONMARK,
    NK_PPTOKEN_COLON,
    NK_PPTOKEN_DEFINED,

    // It's the preprocessor, so we have to let anything we don't
    // understand slide through.
    NK_PPTOKEN_UNKNOWN
};

struct NkppToken
{
    char *str;
    nkuint32_t type;
    nkuint32_t lineNumber;
};

void nkppTokenDestroy(
    struct NkppState *state,
    struct NkppToken *token);

#endif // NK_PPTOKEN_H
