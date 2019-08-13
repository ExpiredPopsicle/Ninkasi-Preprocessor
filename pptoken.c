#include "ppstate.h"
#include "ppcommon.h"
#include "pptoken.h"

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
