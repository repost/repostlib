#ifndef JABCONNECTIONS_H_
#define JABCONNECTIONS_H_

#include "rpl.h"

extern "C" {
#include "purple.h"
}

class jabconnections 
{
public:
  jabconnections();
  ~jabconnections();
  PurpleConnectionUiOps* getUiOps();

private:
  GHashTable *timeoutSignons;
  static PurpleConnectionUiOps ConnectionUiOps;

  static gboolean w_signon(gpointer data);
  gboolean signon(gpointer data);
  static void w_startReconn(PurpleConnection *gc, 
                        PurpleConnectionError reason, const char *text);
  void startReconn(PurpleConnection *gc,
                        PurpleConnectionError reason, const char *text);
  static void w_networkConnected (void);
  void networkConnected (void);
  static void w_accountRemoved(PurpleAccount *account, gpointer user_data);
  void accountRemoved(PurpleAccount *account, gpointer user_data);
  static void w_freeTimeouts(gpointer data);
  void freeTimeouts(gpointer data);
};

#endif
