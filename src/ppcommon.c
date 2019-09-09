#include "ppcommon.h"

void nkppSanityCheck(void)
{
    // If any of these fail, you need to find a way to differentiated
    // this platform or data model from the others and put it in
    // pptypes.h.
    assert(sizeof(nkuint32_t) == 4);
    assert(sizeof(nkint32_t) == 4);
    assert(sizeof(nkuint8_t) == 1);
    assert(sizeof(nkbool) == 4);
}



