#include "wmsp_private.h"

void WMSP_AutoDisconnect(OSMessage msg)
{
    (void)WMSP_DisconnectCore(msg, TRUE, NULL);
}

