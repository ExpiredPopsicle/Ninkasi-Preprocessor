#include "ppcommon.h"

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

struct NkppExpressionStack
{
    nkint32_t *values;
    nkuint32_t capacity;
    nkuint32_t size;
};

struct NkppExpressionStack *nkppExpressionStackCreate(
    struct NkppState *state)
{
    struct NkppExpressionStack *stack;

    stack = nkppMalloc(state, sizeof(struct NkppExpressionStack));
    if(!stack) {
        return NULL;
    }

    stack->values = NULL;
    stack->capacity = 0;
    stack->size = 0;

    return stack;
}

void nkppExpressionStackDestroy(
    struct NkppState *state,
    struct NkppExpressionStack *stack)
{
    nkppFree(state, stack->values);
    nkppFree(state, stack);
}

nkbool nkppExpressionStackPush(
    struct NkppState *state,
    struct NkppExpressionStack *stack,
    nkint32_t value)
{
    nkuint32_t newSize;
    nkuint32_t newCapacity;
    nkbool overflow = nkfalse;

    NK_CHECK_OVERFLOW_UINT_ADD(stack->size, 1, newSize, overflow);
    if(overflow) {
        return nkfalse;
    }

    newCapacity = stack->capacity;

    if(newSize > newCapacity) {

        nkint32_t *newValues;
        nkuint32_t actualAllocationSize;

        // Figure out what the new capacity is going to be. Just
        // double the size of the current allocation, unless we don't
        // have an allocation at all. Then just use one.
        if(newCapacity == 0) {
            newCapacity = 1;
        } else {
            NK_CHECK_OVERFLOW_UINT_MUL(newCapacity, 2, newCapacity, overflow);
        }

        NK_CHECK_OVERFLOW_UINT_MUL(
            newCapacity, sizeof(nkint32_t),
            actualAllocationSize,
            overflow);

        if(overflow) {
            return nkfalse;
        }

        newValues = nkppRealloc(
            state, stack->values,
            actualAllocationSize);

        if(!newValues) {
            return nkfalse;
        }

        stack->values = newValues;
        stack->capacity = newCapacity;
    }

    stack->values[stack->size] = value;
    stack->size = newSize;

    return nktrue;
}

nkbool nkppExpressionStackPop(
    struct NkppExpressionStack *stack)
{
    if(stack->size == 0) {
        return nkfalse;
    }
    stack->size--;
    return nktrue;
}

nkbool nkppExpressionStackPeekTop(
    struct NkppExpressionStack *stack,
    nkint32_t *value)
{
    if(stack->size == 0) {
        return nkfalse;
    }
    *value = stack->values[stack->size - 1];
    return nktrue;
}

nkuint32_t nkppExpressionStackGetSize(
    struct NkppExpressionStack *stack)
{
    return stack->size;
}

nkbool nkppEvaluateExpression_macroDefined(
    struct NkppState *state,
    struct NkppState *expressionState,
    nkint32_t *output)
{
    struct NkppToken *token;
    nkbool ret = nktrue;
    char *identifierStr = NULL;
    struct NkppMacro *macro = NULL;
    nkbool startWithParen = nkfalse;

    token = nkppStateInputGetNextToken(expressionState, nkfalse);
    if(!token) {
        ret = nkfalse;
        goto nkppEvaluateExpression_macroDefined_cleanup;
    }

    // Skip '('.
    if(token->type == NK_PPTOKEN_OPENPAREN) {
        nkppTokenDestroy(state, token);
        token = nkppStateInputGetNextToken(expressionState, nkfalse);
        startWithParen = nktrue;
    }

    // Read identifier.
    if(!token) {
        nkppStateAddError(state, "Expected identifier.");
        ret = nkfalse;
        goto nkppEvaluateExpression_macroDefined_cleanup;
    }
    if(token->type != NK_PPTOKEN_IDENTIFIER) {
        nkppStateAddError2(state, "Expected identifier at: ", token->str);
        ret = nkfalse;
        goto nkppEvaluateExpression_macroDefined_cleanup;
    }
    identifierStr = nkppStrdup(state, token->str);
    nkppTokenDestroy(state, token);
    token = NULL;

    // Skip ')'.
    if(startWithParen) {
        token = nkppStateInputGetNextToken(expressionState, nkfalse);
        if(!token) {
            nkppStateAddError(state, "Expected ')'.");
            ret = nkfalse;
            goto nkppEvaluateExpression_macroDefined_cleanup;
        }
        if(token->type != NK_PPTOKEN_CLOSEPAREN) {
            nkppStateAddError2(state, "Expected ')' at: ", token->str);
            ret = nkfalse;
            goto nkppEvaluateExpression_macroDefined_cleanup;
        }
        nkppTokenDestroy(state, token);
        token = NULL;
    }

    // Look for the macro and set the output.
    macro = nkppStateFindMacro(state, identifierStr);
    if(macro) {
        *output = 1;
    } else {
        *output = 0;
    }

nkppEvaluateExpression_macroDefined_cleanup:

    if(!ret) {
        nkppStateAddError(
            state,
            "Failed to read argument to defined() expression.");
    }

    if(identifierStr) {
        nkppFree(state, identifierStr);
    }
    if(token) {
        nkppTokenDestroy(state, token);
    }
    return ret;
}

nkbool nkppEvaluateExpression_internal(
    struct NkppState *state,
    struct NkppState *expressionState,
    nkint32_t *output,
    nkuint32_t recursionLevel);

nkbool nkppEvaluateExpression_parseValue(
    struct NkppState *state,
    struct NkppState *expressionState,
    nkint32_t *output,
    nkuint32_t recursionLevel)
{
    struct NkppToken *token =
        nkppStateInputGetNextToken(expressionState, nkfalse);
    nkbool ret = nktrue;
    nkuint32_t outputTmp = 0;
    nkint32_t outputSignedTmp = 0;
    nkuint32_t actualRecursionLevel =
        expressionState->recursionLevel + recursionLevel;

    if(actualRecursionLevel > 20) {
        nkppStateAddError(
            expressionState,
            "Arbitrary recursion limit reached in value parser.");
        return nkfalse;
    }

    if(!token) {
        return nkfalse;
    }

    switch(token->type) {

        case NK_PPTOKEN_DEFINED:

            // defined() expression.
            ret = nkppEvaluateExpression_macroDefined(
                state, expressionState,
                output);
            break;

        case NK_PPTOKEN_TILDE:

            // Binary inversion operator.
            ret = nkppEvaluateExpression_parseValue(
                state,
                expressionState,
                &outputSignedTmp,
                recursionLevel + 1);
            *output = ~outputSignedTmp;
            break;

        case NK_PPTOKEN_MINUS:

            // Negation operator.
            ret = nkppEvaluateExpression_parseValue(
                state,
                expressionState,
                &outputSignedTmp,
                recursionLevel + 1);
            *output = -outputSignedTmp;
            break;

        case NK_PPTOKEN_EXCLAMATION:

            // "Not" operator.
            ret = nkppEvaluateExpression_parseValue(
                state,
                expressionState,
                &outputSignedTmp,
                recursionLevel + 1);
            *output = !outputSignedTmp;
            break;

        case NK_PPTOKEN_NUMBER:

            ret = nkppStrtol(token->str, &outputTmp);

            if(outputTmp >= (NK_UINT_MAX >> 1)) {
                char tmp[256];
                sprintf(tmp, "Signed integer overflow: %lu >= %lu",
                    (unsigned long)outputTmp,
                    (unsigned long)(NK_UINT_MAX >> 1));
                nkppStateAddError(expressionState, tmp);
                ret = nkfalse;
            }

            *output = (nkint32_t)outputTmp;
            break;

        case NK_PPTOKEN_OPENPAREN:

            ret = nkppEvaluateExpression_internal(
                state,
                expressionState,
                output, recursionLevel + 1);

            // Skip closing paren.
            if(ret && expressionState->str[expressionState->index] == ')') {
                nkppStateInputSkipChar(expressionState, nkfalse);
            }

            break;

        case NK_PPTOKEN_IDENTIFIER:

            // Identifiers are just undefined macros in this context,
            // with the exception of "defined".
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

nkuint32_t nkppEvaluateExpression_getPrecedence(
    enum NkppTokenType type)
{
    switch(type) {
        case NK_PPTOKEN_ASTERISK:            return 2;  // Multiply
        case NK_PPTOKEN_SLASH:               return 2;  // Divide
        case NK_PPTOKEN_PERCENT:             return 2;  // Modulo
        case NK_PPTOKEN_PLUS:                return 3;  // Add
        case NK_PPTOKEN_MINUS:               return 3;  // Subtract (NOT negation prefix!)
        case NK_PPTOKEN_LEFTSHIFT:           return 4;
        case NK_PPTOKEN_RIGHTSHIFT:          return 4;
        case NK_PPTOKEN_BINARYAND:           return 7;
        case NK_PPTOKEN_BINARYXOR:           return 8;
        case NK_PPTOKEN_BINARYOR:            return 9;
        case NK_PPTOKEN_LOGICALAND:          return 10;
        case NK_PPTOKEN_LOGICALOR:           return 11;
        case NK_PPTOKEN_NOTEQUAL:            return 6;
        case NK_PPTOKEN_COMPARISONEQUALS:    return 6;
        case NK_PPTOKEN_GREATERTHAN:         return 5;
        case NK_PPTOKEN_GREATERTHANOREQUALS: return 5;
        case NK_PPTOKEN_LESSTHAN:            return 5;
        case NK_PPTOKEN_LESSTHANOREQUALS:    return 5;
        case NK_PPTOKEN_QUESTIONMARK:        return 12; // Ternary operator
        default:
            return NK_INVALID_VALUE;
    }
}

nkbool nkppEvaluateExpression_applyOperator(
    struct NkppState *state,
    enum NkppTokenType type,
    nkint32_t a,
    nkint32_t b,
    nkint32_t *result)
{
    switch(type) {

        case NK_PPTOKEN_ASTERISK:
            *result = (a *  b);
            break;

        case NK_PPTOKEN_SLASH:
        case NK_PPTOKEN_PERCENT:
            if(b == 0) {
                nkppStateAddError(state, "Division by zero in expression.");
                return nkfalse;
            }
            if(b == -1 && a == -2147483647 - 1) {
                nkppStateAddError(state, "Result of division cannot be expressed.");
                return nkfalse;
            }
            *result = type == NK_PPTOKEN_SLASH ? (a /  b) : (a %  b);
            break;

        case NK_PPTOKEN_PLUS:
            *result = (a + b);
            break;

        case NK_PPTOKEN_MINUS:
            *result = (a - b);
            break;

        case NK_PPTOKEN_LEFTSHIFT:
            *result = (a << b);
            break;

        case NK_PPTOKEN_RIGHTSHIFT:
            *result = (a >> b);
            break;

        case NK_PPTOKEN_BINARYAND:
            *result = (a & b);
            break;

        case NK_PPTOKEN_BINARYXOR:
            *result = (a ^ b);
            break;

        case NK_PPTOKEN_BINARYOR:
            *result = (a | b);
            break;

        case NK_PPTOKEN_LOGICALAND:
            *result = (a && b);
            break;

        case NK_PPTOKEN_LOGICALOR:
            *result = (a || b);
            break;

        case NK_PPTOKEN_NOTEQUAL:
            *result = (a != b);
            break;

        case NK_PPTOKEN_COMPARISONEQUALS:
            *result = (a == b);
            break;

        case NK_PPTOKEN_GREATERTHAN:
            *result = (a > b);
            break;

        case NK_PPTOKEN_GREATERTHANOREQUALS:
            *result = (a >= b);
            break;

        case NK_PPTOKEN_LESSTHAN:
            *result = (a < b);
            break;

        case NK_PPTOKEN_LESSTHANOREQUALS:
            *result = (a <= b);
            break;

        default:
            *result = NK_INVALID_VALUE;
            return nkfalse;
    }
    return nktrue;
}

nkbool nkppEvaluateExpression_applyStackTop(
    struct NkppState *state,
    struct NkppExpressionStack *valueStack,
    struct NkppExpressionStack *operatorStack)
{
    nkint32_t a = 0;
    nkint32_t b = 0;
    nkint32_t result = 0;
    nkint32_t operatorStackTop = NK_INVALID_VALUE;

    if(!nkppExpressionStackPeekTop(operatorStack, &operatorStackTop)) {
        return nkfalse;
    }

    if(!nkppExpressionStackPop(operatorStack)) {
        return nkfalse;
    }

    if(!nkppExpressionStackPeekTop(valueStack, &a)) {
        return nkfalse;
    }

    if(!nkppExpressionStackPop(valueStack)) {
        return nkfalse;
    }

    if(!nkppExpressionStackPeekTop(valueStack, &b)) {
        return nkfalse;
    }

    if(!nkppExpressionStackPop(valueStack)) {
        return nkfalse;
    }

    if(!nkppEvaluateExpression_applyOperator(
            state,
            operatorStackTop,
            b, a, &result))
    {
        return nkfalse;
    }

    if(!nkppExpressionStackPush(state, valueStack, result)) {
        return nkfalse;
    }

    return nktrue;
}

nkbool nkppEvaluateExpression_atEndOfExpression(
    struct NkppState *expressionState)
{
    if(!expressionState->str[expressionState->index] ||
        expressionState->str[expressionState->index] == ')' ||
        expressionState->str[expressionState->index] == ':')
    {
        return nktrue;
    }
    return nkfalse;
}

nkbool nkppEvaluateExpression_internal(
    struct NkppState *state,
    struct NkppState *expressionState,
    nkint32_t *output,
    nkuint32_t recursionLevel)
{
    nkbool ret = nktrue;
    nkuint32_t actualRecursionLevel =
        state->recursionLevel + recursionLevel;
    struct NkppExpressionStack *valueStack = NULL;
    struct NkppExpressionStack *operatorStack = NULL;
    struct NkppToken *operatorToken = NULL;
    nkint32_t currentOperator = NK_INVALID_VALUE;

    *output = 0;

    // FIXME: Make this less arbitrary.
    if(actualRecursionLevel > 20) {
        nkppStateAddError(state, "Arbitrary recursion limit reached in expression parser.");
        return nkfalse;
    }

    // Allocate stacks.
    valueStack = nkppExpressionStackCreate(state);
    operatorStack = nkppExpressionStackCreate(state);
    if(!valueStack || !operatorStack) {
        ret = nkfalse;
        goto nkppEvaluateExpression_cleanup;
    }

    while(!nkppEvaluateExpression_atEndOfExpression(expressionState)) {

        nkint32_t tmpVal;

        // Parse a value.
        if(!nkppEvaluateExpression_parseValue(
                state,
                expressionState,
                &tmpVal,
                recursionLevel + 1))
        {
            nkppStateAddError(state, "Failed to parse value.");
            ret = nkfalse;
            goto nkppEvaluateExpression_cleanup;
        }
        nkppExpressionStackPush(state, valueStack, tmpVal);

        // Parse an operator.
        nkppStateInputSkipWhitespaceAndComments(expressionState, nkfalse, nkfalse);
        if(!nkppEvaluateExpression_atEndOfExpression(expressionState)) {

            // Parse operator token.
            operatorToken = nkppStateInputGetNextToken(expressionState, nkfalse);
            if(!operatorToken) {
                nkppStateAddError(state, "Failed to parse operator.");
                ret = nkfalse;
                goto nkppEvaluateExpression_cleanup;
            }
            currentOperator = operatorToken->type;

            // Make sure whatever we parsed was actually an operator.
            if(nkppEvaluateExpression_getPrecedence(currentOperator) == NK_INVALID_VALUE) {
                nkppStateAddError2(expressionState, "Bad operator token: ", operatorToken->str);
                ret = nkfalse;
                goto nkppEvaluateExpression_cleanup;
            }

            nkppTokenDestroy(state, operatorToken);
            operatorToken = NULL;

            // Resolve all operators with a higher-precedence than
            // this one that are on the top of the stack.
            while(nkppExpressionStackGetSize(operatorStack)) {

                nkint32_t stackTop = 0;
                nkuint32_t stackPrecedence = 0;
                nkuint32_t currentPrecedence = 0;

                // Compare precedence.
                if(!nkppExpressionStackPeekTop(operatorStack, &stackTop)) {
                    nkppStateAddError(state, "Internal parser error.");
                    ret = nkfalse;
                    goto nkppEvaluateExpression_cleanup;
                }
                stackPrecedence = nkppEvaluateExpression_getPrecedence(stackTop);
                currentPrecedence = nkppEvaluateExpression_getPrecedence(currentOperator);
                if(currentPrecedence <= stackPrecedence) {
                    break;
                }

                // Stack size sanity check.
                if(nkppExpressionStackGetSize(valueStack) < 2) {
                    nkppStateAddError(expressionState, "Expression parse error.");
                    ret = nkfalse;
                    goto nkppEvaluateExpression_cleanup;
                }

                // Apply it.
                if(!nkppEvaluateExpression_applyStackTop(state, valueStack, operatorStack)) {
                    nkppStateAddError(state, "Failed to apply operator.");
                    ret = nkfalse;
                    goto nkppEvaluateExpression_cleanup;
                }
            }

            // Special case for ternary operator handling. Apply this
            // operator immediately instead of pushing onto the stack.
            // This is okay, because it's the lowest precedence and
            // would get applied before anything else anyway.
            if(currentOperator == NK_PPTOKEN_QUESTIONMARK) {

                nkint32_t valueStackTop = 0;
                nkint32_t option1Value = 0;
                nkint32_t option2Value = 0;

                // Get the value at the top of the stack.
                if(!nkppExpressionStackPeekTop(valueStack, &valueStackTop)) {
                    nkppStateAddError(state, "Internal parser error.");
                    ret = nkfalse;
                    goto nkppEvaluateExpression_cleanup;
                }

                // Evaluate the sub-expression to use if it passes.
                if(!nkppEvaluateExpression_internal(
                    state, expressionState,
                    &option1Value, recursionLevel + 1))
                {
                    nkppStateAddError(state, "Failed to evaluate first subexpression in ternary operator.");
                    ret = nkfalse;
                    goto nkppEvaluateExpression_cleanup;
                }

                // Skip colon.
                if(expressionState->str[expressionState->index] != ':') {
                    nkppStateAddError(expressionState, "Expected ':'.");
                    ret = nkfalse;
                    goto nkppEvaluateExpression_cleanup;
                }
                if(!nkppStateInputSkipChar(expressionState, nkfalse)) {
                    ret = nkfalse;
                    goto nkppEvaluateExpression_cleanup;
                }

                // Evaluate the sub-expression to use if it fails.
                if(!nkppEvaluateExpression_internal(
                    state, expressionState,
                    &option2Value, recursionLevel + 1))
                {
                    nkppStateAddError(state, "Failed to evaluate second subexpression in ternary operator.");
                    ret = nkfalse;
                    goto nkppEvaluateExpression_cleanup;
                }

                // Remove the value from the top.
                if(!nkppExpressionStackPop(valueStack)) {
                    ret = nkfalse;
                    goto nkppEvaluateExpression_cleanup;
                }

                // Push the result.
                if(!nkppExpressionStackPush(
                        expressionState, valueStack,
                        valueStackTop ? option1Value : option2Value))
                {
                    ret = nkfalse;
                    goto nkppEvaluateExpression_cleanup;
                }

            } else {

                // Push this operator onto the stack.
                nkppExpressionStackPush(state, operatorStack, currentOperator);

            }
        }

        nkppStateInputSkipWhitespaceAndComments(expressionState, nkfalse, nkfalse);
    }

    // Evaluate the rest of the operators on the operator stack.
    while(nkppExpressionStackGetSize(operatorStack)) {
        if(!nkppEvaluateExpression_applyStackTop(
                expressionState,
                valueStack,
                operatorStack))
        {
            nkppStateAddError(state, "Failed to evaluate final operators.");

            ret = nkfalse;
            goto nkppEvaluateExpression_cleanup;
        }
    }

    // Expression result is the last value on the top of the stack.
    if(nkppExpressionStackGetSize(valueStack) == 1) {
        nkppExpressionStackPeekTop(valueStack, output);
    } else {
        nkppStateAddError(state, "Operators still on stack after expression evaluation.");
        ret = nkfalse;
        goto nkppEvaluateExpression_cleanup;
    }

nkppEvaluateExpression_cleanup:
    if(operatorToken) {
        nkppTokenDestroy(state, operatorToken);
    }
    if(valueStack) {
        nkppExpressionStackDestroy(state, valueStack);
    }
    if(operatorStack) {
        nkppExpressionStackDestroy(state, operatorStack);
    }

    return ret;
}

nkbool nkppEvaluateExpression(
    struct NkppState *state,
    const char *expression,
    nkint32_t *output)
{
    nkbool ret;
    struct NkppState *expressionState = NULL;
    struct NkppState *clonedState = NULL;

    // Make cloned state to preprocess the equation.
    clonedState = nkppStateClone(state, nkfalse, nkfalse);
    if(!clonedState) {
        ret = nkfalse;
        goto nkppEvaluateExpression_outer_cleanup;
    }
    clonedState->preprocessingIfExpression = nktrue;
    if(!nkppStateExecute_internal(clonedState, expression)) {
        ret = nkfalse;
        goto nkppEvaluateExpression_outer_cleanup;
    }

    // Create a state just for reading tokens out of the input string.
    expressionState = nkppStateCreate_internal(
        state->errorState, state->memoryCallbacks);
    if(!expressionState) {
        ret = nkfalse;
        goto nkppEvaluateExpression_outer_cleanup;
    }
    if(!nkppStateSetFilename(expressionState, state->filename)) {
        ret = nkfalse;
        goto nkppEvaluateExpression_outer_cleanup;
    }
    expressionState->lineNumber = state->lineNumber;
    expressionState->str = clonedState->output;
    expressionState->recursionLevel = state->recursionLevel;

    // Execute.
    ret = nkppEvaluateExpression_internal(
        state, expressionState,
        output, 0);

nkppEvaluateExpression_outer_cleanup:
    if(clonedState) {
        nkppStateDestroy_internal(clonedState);
    }
    if(expressionState) {
        nkppStateDestroy_internal(expressionState);
    }

    return ret;
}

#if NK_PP_ENABLETESTS

nkbool nkppTest_expressionTest(void)
{
    struct NkppState *exprState;
    nkbool ret = nktrue;

    NK_PPTEST_SECTION("nkppTest_expressionTest()");

    exprState = nkppStateCreate_internal(NULL, NULL);

    NK_PPTEST_CHECK(exprState);

    if(!exprState) {
        return nkfalse;
    }

    exprState->preprocessingIfExpression = nktrue;

  #define NK_PP_EXPRESSIONTEST_CHECK(x)                     \
    do {                                                    \
        nkint32_t output = 0;                               \
        NK_PPTEST_CHECK(                                    \
            nkppEvaluateExpression(                         \
                exprState, #x, &output) && output == (x));  \
    } while(0)

    NK_PP_EXPRESSIONTEST_CHECK(1 + 1);
    NK_PP_EXPRESSIONTEST_CHECK(1 + 1 * 2);
    NK_PP_EXPRESSIONTEST_CHECK(1 + 1 * -2);
    NK_PP_EXPRESSIONTEST_CHECK(1 + 1 * ~2);
    NK_PP_EXPRESSIONTEST_CHECK(1 + (1 * -2));
    NK_PP_EXPRESSIONTEST_CHECK((1 + 5) * -2);
    NK_PP_EXPRESSIONTEST_CHECK((1 + 5) * (3 * 8));
    NK_PP_EXPRESSIONTEST_CHECK((1 + 5) / (3 * 8));
    NK_PP_EXPRESSIONTEST_CHECK((3 * 8) / (1 + 5));
    NK_PP_EXPRESSIONTEST_CHECK((3/2));
    NK_PP_EXPRESSIONTEST_CHECK(5/(3/2));
    NK_PP_EXPRESSIONTEST_CHECK(-1/(-2147483646 - 2));
    NK_PP_EXPRESSIONTEST_CHECK(1 ? 2 : 3);
    NK_PP_EXPRESSIONTEST_CHECK(1 ? 2 : 3 ? 4 : 5);
    NK_PP_EXPRESSIONTEST_CHECK(1 ? 2 ? 6 : 7 : 3 ? 4 : 5);

    nkppStateDestroy_internal(exprState);

    return ret;
}

#endif // NK_PP_ENABLETESTS
