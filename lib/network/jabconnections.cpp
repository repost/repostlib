#include "jabconnections.h"
#include "rpdebug.h"
#include "acct.h"

#define RECON_DELAY_MIN  8000
#define RECON_DELAY_MAX 60000

static JabConnections *jabconn = NULL;

void JabConnections::w_FreeTimeouts(gpointer data)
{
    if( jabconn )
    {
        jabconn->FreeTimeouts(data);
    }
}

void JabConnections::FreeTimeouts(gpointer data)
{
    g_source_remove(*(guint*)data);
    delete (guint*)data;
}

gboolean JabConnections::w_SignOn(gpointer data)
{
    if( jabconn )
    {
        return jabconn->SignOn(data);
    }
    return false;
}

gboolean JabConnections::SignOn(gpointer data)
{
    PurpleAccount* account = (PurpleAccount*) data;
    LOG(INFO) << "Connect to account " << purple_account_get_username(account);
    if(account)
    {
        purple_account_connect(account);
    }
    g_hash_table_remove(timeout_signons_, data);
    return false;
}

void JabConnections::w_DisconnectReason(PurpleConnection *gc, PurpleConnectionError reason,
                                              const char *text)
{
    if( jabconn )
    {
        jabconn->DisconnectReason(gc, reason, text);
    }
}

void JabConnections::DisconnectReason(PurpleConnection *gc, PurpleConnectionError reason,
                                              const char *text)
{
    PurpleAccount *account = NULL;
    gint delay;

    account = purple_connection_get_account(gc);
    LOG(INFO) << "Account disconnected " << purple_account_get_username(account) 
        << " reason " << reason << " " << text;
    if(!purple_connection_error_is_fatal(reason)) 
    {
        delay = g_random_int_range(RECON_DELAY_MIN, RECON_DELAY_MAX);
        LOG(INFO) << "DisconnectReason add timeout cb";
        gpointer timeid = new guint(g_timeout_add(delay, &JabConnections::w_SignOn, account));
        g_hash_table_insert(timeout_signons_, account, timeid);
    }
    else
    {
        /* 
        ** Should probably only alert users if disconnection is fatal. 
        ** But during testing so far some fatal reasons like bad pass
        ** can be shrugged off by the server as lost conn
        */
    }
    Account acct;
    acct.set_user(std::string(purple_account_get_username(account)));
    /* TODO Fill out account */
    networkuiops_.AccountDisconnect(acct, std::string(text));
}

void JabConnections::w_NetworkConnected(void)
{
    if( jabconn )
    {
        jabconn->NetworkConnected();
    }
}

void JabConnections::NetworkConnected(void)
{
    GList *list, *l;
    l = list = purple_accounts_get_all_active();
    LOG(INFO) << "Connected to network";
    while (l) 
    {
        PurpleAccount *account = (PurpleAccount*)l->data;
        if (purple_account_is_disconnected(account))
        {
            LOG(INFO) << "Checking we are signed into " << purple_account_get_username(account);
            this->SignOn(account);
        }
        l = l->next;
    }
    g_list_free(list);
}

void JabConnections::w_ConnectionConnected(PurpleConnection *gc)
{
    if( jabconn )
    {
        jabconn->ConnectionConnected(gc);
    }
}

void JabConnections::ConnectionConnected(PurpleConnection *gc)
{
    Account ac;
    //networkuiops_.StatusChanged(ac);
}

PurpleConnectionUiOps JabConnections::connectionuiops_ =
{
  NULL, /* connection_connect_progress, */
  &JabConnections::w_ConnectionConnected, /* connection_connected, */
  NULL, /* connection_disconnected, */
  NULL, /* connection_notice, */
  NULL, /* report_disconnect */
  &JabConnections::w_NetworkConnected,
  NULL, /* connection_network_disconnected, */
  &JabConnections::w_DisconnectReason,
  NULL,
  NULL,
  NULL
};

PurpleConnectionUiOps* JabConnections::GetUiOps()
{
    return &JabConnections::connectionuiops_;
}

JabConnections::JabConnections(NetworkUiOps networkuiops):
    networkuiops_(networkuiops)
{
    jabconn = this;
    timeout_signons_ = g_hash_table_new_full(g_direct_hash, g_direct_equal,
            NULL, &JabConnections::w_FreeTimeouts);
}

JabConnections::~JabConnections(void)
{
    g_hash_table_destroy(timeout_signons_);
}
