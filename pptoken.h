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
