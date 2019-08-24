#ifndef NK_PPEXPR_H
#define NK_PPEXPR_H

#include "pptypes.h"

struct NkppState;

/// Evaluate an expression from a string.
nkbool nkppEvaluateExpression(
    struct NkppState *state,
    const char *expression,
    nkint32_t *output,
    nkuint32_t recursionLevel);

#endif // NK_PPEXPR_H
