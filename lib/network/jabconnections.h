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
  static void w_networkConnected (void);
  static void w_startReconn(PurpleConnection *gc, 
                        PurpleConnectionError reason, const char *text);
  static gboolean w_signon(gpointer data);

private:
  GHashTable *timeoutSignons;

  gboolean signon(gpointer data);
  void startReconn(PurpleConnection *gc,
                        PurpleConnectionError reason, const char *text);
  void networkConnected (void);
  static void w_accountRemoved(PurpleAccount *account, gpointer user_data);
  void accountRemoved(PurpleAccount *account, gpointer user_data);
  static void w_freeTimeouts(gpointer data);
  void freeTimeouts(gpointer data);
};

#endif
