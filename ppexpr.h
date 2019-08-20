#ifndef NK_PPEXPR_H
#define NK_PPEXPR_H

#include "pptypes.h"

struct NkppState;

nkbool nkppEvaluateExpression(
    struct NkppState *state,
    const char *expression,
    nkint32_t *output);

#endif // NK_PPEXPR_H
