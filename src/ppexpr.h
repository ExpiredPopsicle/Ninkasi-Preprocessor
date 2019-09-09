#ifndef NK_PPEXPR_H
#define NK_PPEXPR_H

#include "pptypes.h"

struct NkppState;

/// Evaluate an expression from a string. This is used primarily in
/// the #if and #elif directives.
///
/// recursionLevel argument is the level of expression evaluation
/// levels.
nkbool nkppEvaluateExpression(
    struct NkppState *state,
    const char *expression,
    nkint32_t *output);

#if NK_PP_ENABLETESTS
nkbool nkppTest_expressionTest(void);
#endif // NK_PP_ENABLETESTS

#endif // NK_PPEXPR_H
