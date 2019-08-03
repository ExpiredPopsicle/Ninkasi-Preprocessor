#include "ppstate.h"
#include "ppcommon.h"
#include "pptoken.h"

// MEMSAFE
void destroyToken(
    struct PreprocessorState *state,
    struct NkppToken *token)
{
    if(token) {
        if(token->str) {
            nkppFree(state, token->str);
        }
        nkppFree(state, token);
    }
}
