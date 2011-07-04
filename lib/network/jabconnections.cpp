
#include "jabconnections.h"

#define RECON_DELAY_MIN  8000
#define RECON_DELAY_MAX 60000

static jabconnections *jabconn = NULL;

void jabconnections::w_freeTimeouts(gpointer data)
{
  if( jabconn )
  {
    jabconn->freeTimeouts(data);
  }
}

void jabconnections::freeTimeouts(gpointer data)
{
  g_source_remove(*(guint*)data);
  delete (guint*)data;
}

gboolean jabconnections::w_signon(gpointer data)
{
  if( jabconn )
  {
    return jabconn->signon(data);
  }
  return false;
}

gboolean jabconnections::signon(gpointer data)
{
  PurpleAccount* account = (PurpleAccount*) data;
  printf("jabconn signon %s\n",purple_account_get_username(account));
  if(account)
  {
    purple_account_connect(account);
  }
  g_hash_table_remove(this->timeoutSignons, data);
  return false;
}

void jabconnections::w_startReconn(PurpleConnection *gc, PurpleConnectionError reason,
                                              const char *text)
{
  if( jabconn )
  {
    jabconn->startReconn(gc, reason, text);
  }
}

void jabconnections::startReconn(PurpleConnection *gc, PurpleConnectionError reason,
                                              const char *text)
{
  PurpleAccount *account = NULL;
  gint delay;

  account = purple_connection_get_account(gc);
  printf("startreconn acc %s reason %d\n",purple_account_get_username(account), reason);
  if(!purple_connection_error_is_fatal(reason)) 
  {
    delay = g_random_int_range(RECON_DELAY_MIN, RECON_DELAY_MAX);
    printf("startreconn add timeout cb\n");
    gpointer timeid = new guint(g_timeout_add(delay, &jabconnections::w_signon, account));
    g_hash_table_insert(this->timeoutSignons, account, timeid);
                        
  } 
}

void jabconnections::w_networkConnected(void)
{
  if( jabconn )
  {
    jabconn->networkConnected();
  }
}

void jabconnections::networkConnected(void)
{
  GList *list, *l;
  l = list = purple_accounts_get_all_active();
  printf("network connected\n");
  while (l) 
  {
    PurpleAccount *account = (PurpleAccount*)l->data;
    if (purple_account_is_disconnected(account))
    {
      printf("network connected signon %s\n", purple_account_get_username(account));
      this->signon(account);
    }
    l = l->next;
  }
  g_list_free(list);
}

PurpleConnectionUiOps jabconnections::ConnectionUiOps =
{
  NULL, /* connection_connect_progress, */
  NULL, /* connection_connected, */
  NULL, /* connection_disconnected, */
  NULL, /* connection_notice, */
  NULL, /* report_disconnect */
  &jabconnections::w_networkConnected,
  NULL, /* connection_network_disconnected, */
  &jabconnections::w_startReconn,
  NULL,
  NULL,
  NULL
};

PurpleConnectionUiOps* jabconnections::getUiOps()
{
  return &jabconnections::ConnectionUiOps;
}

jabconnections::jabconnections()
{
  jabconn = this;
  timeoutSignons = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                NULL, &jabconnections::w_freeTimeouts);
}

jabconnections::~jabconnections(void)
{
  g_hash_table_destroy(this->timeoutSignons);
}
