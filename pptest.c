#include "ppcommon.h"

#if NK_PP_ENABLETESTS

nkbool nkppTestRun(void)
{
    nkbool ret = nktrue;
    if(!nkppTest_pathTest()) {
        ret = nkfalse;
    }
    return ret;
}

#endif // NK_PP_ENABLETESTS
