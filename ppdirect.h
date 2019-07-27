#ifndef PPDIRECT_H
#define PPDIRECT_H

#include "ppcommon.h"

struct PreprocessorState;

nkbool handleUndef(
    struct PreprocessorState *state,
    const char *restOfLine);

#endif // PPDIRECT_H
