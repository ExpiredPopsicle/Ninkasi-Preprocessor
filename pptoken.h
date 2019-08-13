#ifndef NK_PPTOKEN_H
#define NK_PPTOKEN_H

#include "ppcommon.h"

enum NkppTokenType
{
    NK_PPTOKEN_IDENTIFIER,
    NK_PPTOKEN_QUOTEDSTRING, // str is string with escape characters unconverted and quotes intact!
    NK_PPTOKEN_HASH,         // "#"
    NK_PPTOKEN_DOUBLEHASH,   // "##"
    NK_PPTOKEN_COMMA,
    NK_PPTOKEN_NUMBER,
    NK_PPTOKEN_OPENPAREN,
    NK_PPTOKEN_CLOSEPAREN,

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
