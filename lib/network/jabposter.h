#ifndef JABPOSTER_H_
#define JABPOSTER_H_

#include "rpl.h"
#include "slavenetwork.h"
#include "rpqueue.h"

extern "C" {
#include "purple.h"
#include "glib.h"
}
#define MAX_ACCT 10


typedef struct{
     string user;
} bon_acct;

typedef struct{
    string user;
    string pass;
    string port;
} jab_acct;

class jabposter : public slavenet
{
public:
    jabposter(rpqueue *i);
    ~jabposter();
    int getlinks(rp_link &links, int num);
    void sendpost(Post *post);
    void addlink(rp_link &link);
    void go();
    void add_jab(string user, string pass, string port);
    void add_bon(string user);

private:
    PurpleAccount *acct;
    rpqueue *in_queue;
    int num_bon_accts;
    int num_jab_accts;
    bon_acct bonaccts[MAX_ACCT];
    jab_acct jabaccts[MAX_ACCT];
    void add_bonacct(bon_acct ba);
    void add_jabacct(jab_acct ja);
    void signed_on(PurpleConnection *gc, gpointer jab);
    void connect_to_signals();
    static int authorization_requested(PurpleAccount *account, const char *user);
    static void w_received_im_msg(PurpleAccount *account, char *sender, char *message,
                              PurpleConversation *conv, PurpleMessageFlags flags);
    void received_im_msg(PurpleAccount *account, char *sender, char *message,
                              PurpleConversation *conv, PurpleMessageFlags flags);
    void libpurple();
    void libpurpleloop();
    static void *start_thread(void *obj);
};

#endif
