#include "ppcommon.h"
#include "pptoken.h"

void destroyToken(struct PreprocessorToken *token)
{
    freeWrapper(token->str);
    freeWrapper(token);
}
