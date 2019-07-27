#include "ppstate.h"
#include "pptoken.h"
#include "ppdirect.h"

nkbool handleUndef(
    struct PreprocessorState *state,
    const char *restOfLine)
{
    nkbool ret = nktrue;
    struct PreprocessorState *directiveParseState = createPreprocessorState();
    struct PreprocessorToken *identifierToken;

    directiveParseState->str = restOfLine;

    // Get identifier.
    identifierToken = getNextToken(directiveParseState, nkfalse);

    if(identifierToken->type == NK_PPTOKEN_IDENTIFIER) {

        if(preprocessorStateDeleteMacro(state, identifierToken->str)) {

            // Success!

        } else {

            preprocessorStateAddError(state, "Cannot delete macro.");
            ret = nkfalse;

        }

    } else {

        preprocessorStateAddError(state, "Invalid identifier.");
        ret = nkfalse;

    }

    destroyToken(identifierToken);

    destroyPreprocessorState(directiveParseState);

    return ret;
}
