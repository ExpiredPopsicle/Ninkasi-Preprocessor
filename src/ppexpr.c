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


typedef signed long int NkppExpressionStackValue_intType;
typedef unsigned long int NkppExpressionStackValue_uintType;

// These limits make assumptions about two's complement number
// formats, which C89 doesn't actuall specify, but everyone uses
// anyway.
#define NK_PPEXPRESSIONSTACKVALUE_UINT_MAX (~(NkppExpressionStackValue_uintType)0)
#define NK_PPEXPRESSIONSTACKVALUE_INT_MAX (NK_PPEXPRESSIONSTACKVALUE_UINT_MAX >> 1)

struct NkppExpressionStackValue
{
    union {
        NkppExpressionStackValue_uintType uintValue;
        NkppExpressionStackValue_intType intValue;
        enum NkppTokenType operatorType;
    };
    nkbool signedInt;
};

struct NkppExpressionStack
{
    struct NkppExpressionStackValue *values;
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
    struct NkppExpressionStackValue value)
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

        struct NkppExpressionStackValue *newValues;
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
            newCapacity,
            sizeof(struct NkppExpressionStackValue),
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
    struct NkppExpressionStackValue *value)
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
// ----------------------------------------------------------------------

nkbool nkppExpressionStrtol(
    const char *str,
    NkppExpressionStackValue_uintType *out)
{
    NkppExpressionStackValue_uintType base = 10;

    *out = 0;

    if(!str || !str[0]) {
        return nkfalse;
    }

    // Skip whitespace.
    while(nkppIsWhitespace(str[0])) {
        str++;
    }

    // Determine base.
    if(str[0] == '0') {

        // Default to octal with no other info.
        base = 8;
        str++;

        if(str[0] == 'x' || str[0] == 'X') {

            // Hex.
            base = 16;
            str++;

        } else if(str[0] == 'b' || str[0] == 'B') {

            // Binary.
            base = 2;
            str++;

        }
    }

    while(str[0]) {

        NkppExpressionStackValue_uintType digit =
            (NkppExpressionStackValue_uintType)nkppParseDigit(str[0]);

        // 'L' and 'U' are postfix type specifiers. If we see them,
        // it's time to bail out of the actual number parsing.
        if(str[0] == 'L' || str[0] == 'l' ||
            str[0] == 'U' || str[0] == 'u')
        {
            break;
        }

        if(digit >= base || digit == NK_INVALID_VALUE) {
            return nkfalse;
        }

        *out = *out * base;
        *out += digit;

        str++;
    }

    return nktrue;
}

// ----------------------------------------------------------------------

nkbool nkppEvaluateExpression_macroDefined(
    struct NkppState *state,
    struct NkppState *expressionState,
    struct NkppExpressionStackValue *output)
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
    identifierStr = token->str;
    token->str = NULL;
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
    output->signedInt = nkfalse;
    if(macro) {
        output->uintValue = 1;
    } else {
        output->uintValue = 0;
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
    struct NkppExpressionStackValue *output,
    nkuint32_t recursionLevel);

nkbool nkppEvaluateExpression_parseValue(
    struct NkppState *state,
    struct NkppState *expressionState,
    struct NkppExpressionStackValue *output,
    nkuint32_t recursionLevel)
{
    struct NkppToken *token =
        nkppStateInputGetNextToken(expressionState, nkfalse);
    nkbool ret = nktrue;
    NkppExpressionStackValue_uintType outputTmp = 0;
    struct NkppExpressionStackValue outputValueTmp = {0};
    nkuint32_t actualRecursionLevel =
        expressionState->recursionLevel + recursionLevel;

    if(actualRecursionLevel > 20) {
        nkppStateAddError(
            expressionState,
            "Arbitrary recursion limit reached in value parser.");
        if(token) {
            nkppTokenDestroy(state, token);
        }
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
                &outputValueTmp,
                recursionLevel + 1);
            *output = outputValueTmp;

            if(outputValueTmp.signedInt) {
                output->intValue = ~output->intValue;
            } else {
                output->uintValue = ~output->uintValue;
            }

            break;

        case NK_PPTOKEN_MINUS:

            // Negation operator.
            ret = nkppEvaluateExpression_parseValue(
                state,
                expressionState,
                &outputValueTmp,
                recursionLevel + 1);
            *output = outputValueTmp;

            // Check overflow.
            if(!output->signedInt && output->uintValue > NK_PPEXPRESSIONSTACKVALUE_INT_MAX) {
                nkppStateAddError(state, "Integer overflow converting to signed from unsigned int.");
                ret = nkfalse;
            }

            // Negate a signed int or convert to signed if unsigned.
            if(outputValueTmp.signedInt) {
                output->intValue = -output->intValue;
            } else {
                output->intValue = -output->uintValue;
                output->signedInt = nktrue;
            }

            break;

        case NK_PPTOKEN_EXCLAMATION:

            // "Not" operator.
            ret = nkppEvaluateExpression_parseValue(
                state,
                expressionState,
                &outputValueTmp,
                recursionLevel + 1);
            *output = outputValueTmp;

            if(outputValueTmp.signedInt) {
                output->intValue = !output->intValue;
            } else {
                output->uintValue = !output->uintValue;
            }

            break;

        case NK_PPTOKEN_NUMBER:

            ret = nkppExpressionStrtol(
                token->str, &outputTmp);

            output->signedInt = nkfalse;
            output->uintValue = outputTmp;
            break;

        case NK_PPTOKEN_OPENPAREN:

            ret = nkppEvaluateExpression_internal(
                state,
                expressionState,
                output,
                recursionLevel + 1);

            // Skip closing paren.
            if(ret && expressionState->str[expressionState->index] == ')') {
                nkppStateInputSkipChar(expressionState, nkfalse);
            }

            break;

        case NK_PPTOKEN_IDENTIFIER:

            // Identifiers are just undefined macros in this context,
            // with the exception of "defined".
            output->signedInt = nkfalse;
            output->uintValue = 0;
            break;

        default:

            nkppStateAddError(expressionState, "Expected value token.");
            ret = nkfalse;
            output->signedInt = 0;
            output->uintValue = nkfalse;
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
    struct NkppExpressionStackValue a,
    struct NkppExpressionStackValue b,
    struct NkppExpressionStackValue *result)
{
    nkbool ret = nktrue;

    // Handle type promotions.
    if(a.signedInt && !b.signedInt) {
        if(b.uintValue > NK_PPEXPRESSIONSTACKVALUE_INT_MAX) {
            nkppStateAddError(state, "Integer overflow converting to signed from unsigned int.");
            ret = nkfalse;
        }
        b.signedInt = nktrue;
        b.intValue = (NkppExpressionStackValue_intType)b.uintValue;
    }
    if(b.signedInt && !a.signedInt) {
        if(a.uintValue > NK_PPEXPRESSIONSTACKVALUE_INT_MAX) {
            nkppStateAddError(state, "Integer overflow converting to signed from unsigned int.");
            ret = nkfalse;
        }
        a.signedInt = nktrue;
        a.intValue = (NkppExpressionStackValue_intType)a.uintValue;
    }

    result->signedInt = a.signedInt;

    switch(type) {

        case NK_PPTOKEN_ASTERISK:
            if(a.signedInt) {
                result->intValue = a.intValue * b.intValue;
            } else {
                result->uintValue = a.uintValue * b.uintValue;
            }
            break;

        case NK_PPTOKEN_SLASH:
        case NK_PPTOKEN_PERCENT:
            if(b.uintValue == 0) { // Same for uint and int.
                nkppStateAddError(state, "Division by zero in expression.");
                return nkfalse;
            }
            if(a.signedInt && b.intValue == -1 && a.intValue == -2147483647 - 1) {
                nkppStateAddError(state, "Result of division cannot be expressed.");
                return nkfalse;
            }

            if(a.signedInt) {
                result->intValue =
                    type == NK_PPTOKEN_SLASH ?
                    (a.intValue /  b.intValue) :
                    (a.intValue %  b.intValue);
            } else {
                result->uintValue =
                    type == NK_PPTOKEN_SLASH ?
                    (a.uintValue /  b.uintValue) :
                    (a.uintValue %  b.uintValue);
            }

            break;

        case NK_PPTOKEN_PLUS:
            if(a.signedInt) {
                result->intValue = a.intValue + b.intValue;
            } else {
                result->uintValue = a.uintValue + b.uintValue;
            }
            break;

        case NK_PPTOKEN_MINUS:
            if(a.signedInt) {
                result->intValue = a.intValue - b.intValue;
            } else {

                if(a.uintValue > NK_PPEXPRESSIONSTACKVALUE_INT_MAX ||
                    b.uintValue > NK_PPEXPRESSIONSTACKVALUE_INT_MAX)
                {
                    nkppStateAddError(state, "Integer overflow converting to signed from unsigned int.");
                    ret = nkfalse;
                }

                if(b.uintValue > a.uintValue) {
                    result->intValue =
                        (NkppExpressionStackValue_intType)a.uintValue -
                        (NkppExpressionStackValue_intType)b.uintValue;
                    result->signedInt = nktrue;
                }
            }
            break;

        case NK_PPTOKEN_LEFTSHIFT:
            if(a.signedInt) {
                result->intValue = a.intValue << b.intValue;
            } else {
                result->uintValue = a.uintValue << b.uintValue;
            }
            break;

        case NK_PPTOKEN_RIGHTSHIFT:
            if(a.signedInt) {
                result->intValue = a.intValue >> b.intValue;
            } else {
                result->uintValue = a.uintValue >> b.uintValue;
            }
            break;

        case NK_PPTOKEN_BINARYAND:
            if(a.signedInt) {
                result->intValue = a.intValue & b.intValue;
            } else {
                result->uintValue = a.uintValue & b.uintValue;
            }
            break;

        case NK_PPTOKEN_BINARYXOR:
            if(a.signedInt) {
                result->intValue = a.intValue ^ b.intValue;
            } else {
                result->uintValue = a.uintValue ^ b.uintValue;
            }
            break;

        case NK_PPTOKEN_BINARYOR:
            if(a.signedInt) {
                result->intValue = a.intValue | b.intValue;
            } else {
                result->uintValue = a.uintValue | b.uintValue;
            }
            break;

        case NK_PPTOKEN_LOGICALAND:
            if(a.signedInt) {
                result->intValue = a.intValue && b.intValue;
            } else {
                result->uintValue = a.uintValue && b.uintValue;
            }
            break;

        case NK_PPTOKEN_LOGICALOR:
            if(a.signedInt) {
                result->intValue = a.intValue || b.intValue;
            } else {
                result->uintValue = a.uintValue || b.uintValue;
            }
            break;

        case NK_PPTOKEN_NOTEQUAL:
            if(a.signedInt) {
                result->intValue = (a.intValue != b.intValue);
            } else {
                result->uintValue = (a.uintValue != b.uintValue);
            }
            break;

        case NK_PPTOKEN_COMPARISONEQUALS:
            if(a.signedInt) {
                result->intValue = (a.intValue == b.intValue);
            } else {
                result->uintValue = (a.uintValue == b.uintValue);
            }
            break;

        case NK_PPTOKEN_GREATERTHAN:
            if(a.signedInt) {
                result->intValue = a.intValue > b.intValue;
            } else {
                result->uintValue = a.uintValue > b.uintValue;
            }
            break;

        case NK_PPTOKEN_GREATERTHANOREQUALS:
            if(a.signedInt) {
                result->intValue = a.intValue >= b.intValue;
            } else {
                result->uintValue = a.uintValue >= b.uintValue;
            }
            break;

        case NK_PPTOKEN_LESSTHAN:
            if(a.signedInt) {
                result->intValue = a.intValue < b.intValue;
            } else {
                result->uintValue = a.uintValue < b.uintValue;
            }
            break;

        case NK_PPTOKEN_LESSTHANOREQUALS:
            if(a.signedInt) {
                result->intValue = a.intValue <= b.intValue;
            } else {
                result->uintValue = a.uintValue <= b.uintValue;
            }
            break;

        default:
            result->uintValue = NK_INVALID_VALUE;
            result->signedInt = nkfalse;
            return nkfalse;
    }

    return nktrue && ret;
}

nkbool nkppEvaluateExpression_applyStackTop(
    struct NkppState *state,
    struct NkppExpressionStack *valueStack,
    struct NkppExpressionStack *operatorStack)
{
    struct NkppExpressionStackValue a = {0};
    struct NkppExpressionStackValue b = {0};
    struct NkppExpressionStackValue result = {0};
    struct NkppExpressionStackValue operatorStackTop = {0};

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
            operatorStackTop.operatorType,
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
    if(!expressionState->str) {
        return nktrue;
    }

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
    struct NkppExpressionStackValue *output,
    nkuint32_t recursionLevel)
{
    nkbool ret = nktrue;
    nkuint32_t actualRecursionLevel =
        state->recursionLevel + recursionLevel;
    struct NkppExpressionStack *valueStack = NULL;
    struct NkppExpressionStack *operatorStack = NULL;
    struct NkppToken *operatorToken = NULL;
    enum NkppTokenType currentOperator = NK_INVALID_VALUE;

    output->signedInt = nkfalse;
    output->uintValue = 0;

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

        struct NkppExpressionStackValue tmpVal = {0};

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

                struct NkppExpressionStackValue stackTop = {0};
                nkuint32_t stackPrecedence = 0;
                nkuint32_t currentPrecedence = 0;

                // Compare precedence.
                if(!nkppExpressionStackPeekTop(operatorStack, &stackTop)) {
                    nkppStateAddError(state, "Internal parser error.");
                    ret = nkfalse;
                    goto nkppEvaluateExpression_cleanup;
                }
                stackPrecedence = nkppEvaluateExpression_getPrecedence(stackTop.operatorType);
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

                struct NkppExpressionStackValue valueStackTop = {0};
                struct NkppExpressionStackValue option1Value = {0};
                struct NkppExpressionStackValue option2Value = {0};

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
                        valueStackTop.uintValue ? option1Value : option2Value))
                {
                    ret = nkfalse;
                    goto nkppEvaluateExpression_cleanup;
                }

            } else {

                // Push this operator onto the stack.
                struct NkppExpressionStackValue tmp = {0};
                tmp.operatorType = currentOperator;
                nkppExpressionStackPush(state, operatorStack, tmp);

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
    struct NkppExpressionStackValue result = {0};

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
        &result, 0);

    // FIXME: We may be converting down from a more extensive type
    // here, or from a uint to an int, but we only check to see if
    // it's zero outside of this, so it's not a big issue..
    *output = result.intValue;

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

    NK_PP_EXPRESSIONTEST_CHECK(-1 == -1);
    NK_PP_EXPRESSIONTEST_CHECK(-1 == 1 - 2);
    // NK_PP_EXPRESSIONTEST_CHECK(0xffffffff == -1);

    // NK_PP_EXPRESSIONTEST_CHECK(0xffffffff != -2);
    NK_PP_EXPRESSIONTEST_CHECK(1234 == 5678);
    NK_PP_EXPRESSIONTEST_CHECK(0xffffffff);
    NK_PP_EXPRESSIONTEST_CHECK(0xfffffffe);
    NK_PP_EXPRESSIONTEST_CHECK(-0xefffffff);

    NK_PP_EXPRESSIONTEST_CHECK(1234U - 1234L);

    nkppStateDestroy_internal(exprState);

    return ret;
}

#endif // NK_PP_ENABLETESTS
