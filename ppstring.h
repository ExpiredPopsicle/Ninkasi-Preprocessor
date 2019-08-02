#ifndef NK_PPSTRING_H
#define NK_PPSTRING_H

#include "ppcommon.h"

char *deleteBackslashNewlines(struct PreprocessorState *state, const char *str);
char *stripCommentsAndTrim(struct PreprocessorState *state, const char *in);

#endif // NK_PPSTRING_H

