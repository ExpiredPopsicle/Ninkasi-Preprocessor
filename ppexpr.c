#include "ppcommon.h"

nkbool nkppEvaluateExpression(
    struct NkppState *state,
    const char *expression,
    nkint32_t *output)
{
    struct NkppState *expressionState;
    nkbool ret = nkfalse;

    expressionState = nkppStateCreate(
        state->errorState, state->memoryCallbacks);
    if(!expressionState) {
        goto nkppEvaluateExpression_cleanup;
    }

    expressionState->str = expression;

    // TODO

nkppEvaluateExpression_cleanup:
    if(expressionState) {
        nkppStateDestroy(expressionState);
    }

    return ret;
}

// Operators and "stuff" to support...
//   Parenthesis                  ( 0) (treat as value)
//   defined()                    ( 0) (treat as value)
//   Prefix operators...
//     "-" negation               ( 1) (handled in value parsing)
//     "~" binary inverse         ( 1) (handled in value parsing)
//     "!" not                    ( 1) (handled in value parsing)
//   Math...
//     "*" multiply               ( 2)
//     "/" divide                 ( 2)
//     "%" modulo                 ( 2)
//     "+" add                    ( 3)
//     "-" subtract               ( 3)
//   Binary...
//     "<<" left-shift            ( 4)
//     ">>" right-shift           ( 4)
//     "&" binary and             ( 7)
//     "^" xor                    ( 8)
//     "|" binary or              ( 9)
//   Logic...
//     "||" logical or            (11)
//     "&&" logical and           (10)
//     "!=" not-equal             ( 6)
//     "==" equal                 ( 6)
//     ">" greater-than           ( 5)
//     ">=" greater-than or equal ( 5)
//     "<" less-than              ( 5)
//     "<=" less-than or equal    ( 5)
//   Ternary (?:) operator        (12)

