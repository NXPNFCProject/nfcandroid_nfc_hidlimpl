#include "SeEvtCallback.h"
#include "eSEClient.h"

void SeEvtCallback::evtCallback(__attribute__((unused)) SESTATUS evt) {
    sendeSEUpdateState(ESE_JCOP_UPDATE_COMPLETED);
    seteSEClientState(ESE_UPDATE_COMPLETED);
    eSEUpdate_SeqHandler();
return;
}