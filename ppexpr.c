#include "ppcommon.h"

nkbool nkppEvaluateExpression_parseValue(
    struct NkppState *expressionState,
    nkint32_t *output,
    nkuint32_t recursionLevel)
{
    struct NkppToken *token =
        nkppStateInputGetNextToken(expressionState, nkfalse);
    nkbool ret = nktrue;
    nkuint32_t outputTmp = 0;
    nkint32_t outputSignedTmp = 0;

    // FIXME: Check recursion limit!

    printf("nkppEvaluateExpression_parseValue: %s\n", expressionState->str);

    if(!token) {
        return nkfalse;
    }

    switch(token->type) {

        case NK_PPTOKEN_TILDE:
            ret = nkppEvaluateExpression_parseValue(
                expressionState,
                &outputSignedTmp,
                recursionLevel + 1);
            *output = ~outputSignedTmp;
            break;

        case NK_PPTOKEN_MINUS:
            ret = nkppEvaluateExpression_parseValue(
                expressionState,
                &outputSignedTmp,
                recursionLevel + 1);
            *output = -outputSignedTmp;
            break;

        case NK_PPTOKEN_NUMBER:

            // FIXME: Check overflow.

            ret = nkppStrtol(token->str, &outputTmp);

            if(!ret) {
                printf("Number fail\n");
            } else {
                printf("Number success\n");
            }

            *output = (nkint32_t)outputTmp;
            break;

        case NK_PPTOKEN_OPENPAREN:

            ret = nkppEvaluateExpression(
                expressionState,
                expressionState->str + expressionState->index,
                output, recursionLevel + 1);

            if(!ret) {
                printf("Open paren fail\n");
            } else {
                printf("Open paren success\n");
            }

            break;

        case NK_PPTOKEN_IDENTIFIER:

            // Identifiers are just undefined macros in this context,
            // with the exception of "defined", which we'll do the
            // logic for here.

            // FIXME: Handle "defined()"

            *output = 0;
            break;

        default:

            nkppStateAddError(expressionState, "Expected value token.");
            ret = nkfalse;
            break;
    }

    if(!ret) {
        nkppStateAddError(expressionState, "Failed to parse value token.");
    }

    nkppTokenDestroy(expressionState, token);

    return ret;
}

nkbool nkppEvaluateExpression(
    struct NkppState *state,
    const char *expression,
    nkint32_t *output,
    nkuint32_t recursionLevel)
{
    struct NkppState *expressionState;
    nkbool ret = nktrue;
    nkuint32_t actualRecursionLevel =
        state->recursionLevel + recursionLevel;

    // FIXME: Make this less arbitrary.
    if(actualRecursionLevel > 20) {
        nkppStateAddError(state, "Arbitrary recursion limit reached in expression parser.");
        return nkfalse;
    }

    // Create a state just for reading tokens out of the input string.
    expressionState = nkppStateCreate(
        state->errorState, state->memoryCallbacks);
    if(!expressionState) {
        ret = nkfalse;
        goto nkppEvaluateExpression_cleanup;
    }
    expressionState->str = expression;

    // Parse a value.
    {
        nkint32_t tmpVal;
        if(!nkppEvaluateExpression_parseValue(
                expressionState, &tmpVal, recursionLevel + 1))
        {
            ret = nkfalse;
            goto nkppEvaluateExpression_cleanup;
        }

        printf("Value parsed: %ld\n", (long)tmpVal);
        *output = tmpVal;
    }

    // Parse an operator.

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

