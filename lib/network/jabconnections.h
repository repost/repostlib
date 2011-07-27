#ifndef JABCONNECTIONS_H_
#define JABCONNECTIONS_H_

#include "rpl.h"

extern "C" {
#include "purple.h"
}

class NetworkUiOps;

class JabConnections 
{
public:
    JabConnections(NetworkUiOps networkuiops);
    ~JabConnections();
    PurpleConnectionUiOps* GetUiOps();

private:
    GHashTable *timeout_signons_;
    NetworkUiOps networkuiops_;
    static PurpleConnectionUiOps connectionuiops_;

    static void w_ConnectionConnected(PurpleConnection *gc);
    void ConnectionConnected(PurpleConnection *gc);

    static gboolean w_SignOn(gpointer data);
    gboolean SignOn(gpointer data);

    static void w_DisconnectReason(PurpleConnection *gc, 
            PurpleConnectionError reason, const char *text);
    void DisconnectReason(PurpleConnection *gc,
            PurpleConnectionError reason, const char *text);

    static void w_NetworkConnected (void);
    void NetworkConnected (void);

    static void w_FreeTimeouts(gpointer data);
    void FreeTimeouts(gpointer data);
};

#endif
