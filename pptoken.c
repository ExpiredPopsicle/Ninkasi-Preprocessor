#include "ppcommon.h"

void nkppTokenDestroy(
    struct NkppState *state,
    struct NkppToken *token)
{
    if(token) {
        if(token->str) {
            nkppFree(state, token->str);
        }
        nkppFree(state, token);
    }
}
