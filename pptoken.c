#include "ppcommon.h"
#include "pptoken.h"

// MEMSAFE
void destroyToken(struct PreprocessorToken *token)
{
    if(token) {
        if(token->str) {
            freeWrapper(token->str);
        }
        freeWrapper(token);
    }
}
