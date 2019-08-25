#include "ppcommon.h"

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

    // Skip '('.
    token = nkppStateInputGetNextToken(expressionState, nkfalse);
    if(!token) {
        ret = nkfalse;
        goto nkppEvaluateExpression_macroDefined_cleanup;
    }
    if(token->type != NK_PPTOKEN_OPENPAREN) {
        ret = nkfalse;
        goto nkppEvaluateExpression_macroDefined_cleanup;
    }
    nkppTokenDestroy(state, token);

    // Read identifier.
    token = nkppStateInputGetNextToken(expressionState, nkfalse);
    if(!token) {
        ret = nkfalse;
        goto nkppEvaluateExpression_macroDefined_cleanup;
    }
    if(token->type != NK_PPTOKEN_IDENTIFIER) {
        ret = nkfalse;
        goto nkppEvaluateExpression_macroDefined_cleanup;
    }
    identifierStr = nkppStrdup(state, token->str);
    nkppTokenDestroy(state, token);

    // Skip ')'.
    token = nkppStateInputGetNextToken(expressionState, nkfalse);
    if(!token) {
        ret = nkfalse;
        goto nkppEvaluateExpression_macroDefined_cleanup;
    }
    if(token->type != NK_PPTOKEN_CLOSEPAREN) {
        ret = nkfalse;
        goto nkppEvaluateExpression_macroDefined_cleanup;
    }
    nkppTokenDestroy(state, token);
    token = NULL;

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
            "Failed to read argument to defined() expression.\n");
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

    printf("nkppEvaluateExpression_parseValue: %s\n",
        expressionState->str + expressionState->index);

    if(!token) {
        return nkfalse;
    }

    switch(token->type) {

        case NK_PPTOKEN_DEFINED:

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
                nkppStateAddError(expressionState, "Signed integer overflow.");
                ret = nkfalse;
            }

            *output = (nkint32_t)outputTmp;
            break;

        case NK_PPTOKEN_OPENPAREN:

            printf("Parsing subexpression: %s\n",
                expressionState->str + expressionState->index);

            ret = nkppEvaluateExpression_internal(
                expressionState,
                expressionState,
                output, recursionLevel + 1);

            printf("Parsed subexpression: %s\n",
                expressionState->str + expressionState->index);

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

        case NK_PPTOKEN_PLUS:                *result = (a +  b); break;  // Add
        case NK_PPTOKEN_MINUS:               *result = (a -  b); break;  // Subtract (NOT negation prefix!)
        case NK_PPTOKEN_LEFTSHIFT:           *result = (a << b); break;
        case NK_PPTOKEN_RIGHTSHIFT:          *result = (a >> b); break;
        case NK_PPTOKEN_BINARYAND:           *result = (a &  b); break;
        case NK_PPTOKEN_BINARYXOR:           *result = (a ^  b); break;
        case NK_PPTOKEN_BINARYOR:            *result = (a |  b); break;
        case NK_PPTOKEN_LOGICALAND:          *result = (a && b); break;
        case NK_PPTOKEN_LOGICALOR:           *result = (a || b); break;
        case NK_PPTOKEN_NOTEQUAL:            *result = (a != b); break;
        case NK_PPTOKEN_COMPARISONEQUALS:    *result = (a == b); break;
        case NK_PPTOKEN_GREATERTHAN:         *result = (a >  b); break;
        case NK_PPTOKEN_GREATERTHANOREQUALS: *result = (a >= b); break;
        case NK_PPTOKEN_LESSTHAN:            *result = (a <  b); break;
        case NK_PPTOKEN_LESSTHANOREQUALS:    *result = (a <= b); break;
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

    while(expressionState->str[expressionState->index] &&
        expressionState->str[expressionState->index] != ')')
    {
        // Parse a value.
        {
            nkint32_t tmpVal;
            if(!nkppEvaluateExpression_parseValue(
                    state,
                    expressionState,
                    &tmpVal,
                    recursionLevel + 1))
            {
                ret = nkfalse;
                goto nkppEvaluateExpression_cleanup;
            }

            nkppExpressionStackPush(state, valueStack, tmpVal);
        }

        // Parse an operator.
        nkppStateInputSkipWhitespaceAndComments(expressionState, nkfalse, nkfalse);
        if(expressionState->str[expressionState->index] &&
            expressionState->str[expressionState->index] != ')')
        {
            printf("Hrmmm... %s\n", expressionState->str + expressionState->index);

            // Parse next operator.
            operatorToken = nkppStateInputGetNextToken(expressionState, nkfalse);
            if(!operatorToken) {
                ret = nkfalse;
                goto nkppEvaluateExpression_cleanup;
            }
            currentOperator = operatorToken->type;
            nkppTokenDestroy(state, operatorToken);
            operatorToken = NULL;

            // Make sure whatever we parsed was a good operator.
            if(nkppEvaluateExpression_getPrecedence(currentOperator) == NK_INVALID_VALUE) {
                nkppStateAddError(expressionState, "Bad operator token.");
                ret = nkfalse;
                goto nkppEvaluateExpression_cleanup;
            }

            // Resolve all higher-precedence operators.
            while(nkppExpressionStackGetSize(operatorStack)) {

                nkint32_t stackTop;
                nkuint32_t stackPrecedence;
                nkuint32_t currentPrecedence;

                if(!nkppExpressionStackPeekTop(operatorStack, &stackTop)) {
                    ret = nkfalse;
                    goto nkppEvaluateExpression_cleanup;
                }

                stackPrecedence = nkppEvaluateExpression_getPrecedence(stackTop);
                currentPrecedence = nkppEvaluateExpression_getPrecedence(currentOperator);

                if(currentPrecedence <= stackPrecedence) {
                    break;
                }

                if(nkppExpressionStackGetSize(valueStack) < 2) {
                    nkppStateAddError(expressionState, "Expression parse error.");
                    ret = nkfalse;
                    goto nkppEvaluateExpression_cleanup;
                }

                if(!nkppEvaluateExpression_applyStackTop(state, valueStack, operatorStack)) {
                    ret = nkfalse;
                    goto nkppEvaluateExpression_cleanup;
                }


                printf("Applied operator\n");
            }

            // Push this one onto the stack.
            nkppExpressionStackPush(state, operatorStack, currentOperator);
        }

        nkppStateInputSkipWhitespaceAndComments(expressionState, nkfalse, nkfalse);
    }

    if(expressionState->str[expressionState->index] == ')') {
        nkppStateInputSkipChar(expressionState, nkfalse);
    }

    while(nkppExpressionStackGetSize(operatorStack)) {

        printf("Applying final operator %lu %lu\n",
            (long)nkppExpressionStackGetSize(operatorStack),
            (long)nkppExpressionStackGetSize(valueStack));

        if(!nkppEvaluateExpression_applyStackTop(
                expressionState,
                valueStack,
                operatorStack))
        {
            ret = nkfalse;
            goto nkppEvaluateExpression_cleanup;
        }
    }

    printf("Operators parsed: %lu\n", operatorStack ? (long)operatorStack->size : 0);
    printf("Values parsed: %lu\n", valueStack ? (long)valueStack->size : 0);

    if(nkppExpressionStackGetSize(valueStack) == 1) {
        nkppExpressionStackPeekTop(valueStack, output);
    } else {
        ret = nkfalse;
        goto nkppEvaluateExpression_cleanup;
    }

    printf("Output: %ld\n", (long)*output);

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
    nkint32_t *output,
    nkuint32_t recursionLevel)
{
    nkbool ret;

    struct NkppState *expressionState = NULL;

    // Create a state just for reading tokens out of the input string.
    expressionState = nkppStateCreate(
        state->errorState, state->memoryCallbacks);
    if(!expressionState) {
        ret = nkfalse;
        goto nkppEvaluateExpression_outer_cleanup;
    }
    expressionState->str = expression;
    expressionState->recursionLevel = state->recursionLevel;

    // Execute.
    ret = nkppEvaluateExpression_internal(
        state, expressionState,
        output, recursionLevel);

nkppEvaluateExpression_outer_cleanup:
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

