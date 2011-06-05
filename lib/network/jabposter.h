#ifndef JABPOSTER_H_
#define JABPOSTER_H_

#include "rpl.h"
#include "link.h"
#include "slavenetwork.h"
#include "rpqueue.h"

#include <string>

extern "C" {
#include "purple.h"
}

#define MAXACCTS 10

class jabposter : public slavenet
{
public:
    jabposter(rpqueue* rq);
    ~jabposter();
    int getlinks(Link* links, int num);
    int getaccounts(Account* accts, int num);
    void addBonjour(string user);
    void addJabber(string user, string pass);
    void sendpost(Post* post);
    void addlink(Link& link);
    void rmlink(Link& link);
    void go();
    std::string get_repostdir();
    
private:
    PurpleAccount *acct;
    rpqueue *in_queue;
    Account pendaccts[MAXACCTS];
    std::string repostdir;
    void libpurpleDiag();
    void connect_to_signals();
    static int authorization_requested(PurpleAccount* account, const char* user);
    static void w_accountSignedOff(PurpleConnection *gc, void *data);
    static void w_connError(PurpleConnection *gc, PurpleConnectionError err, const gchar *desc);
    void accountSignedOff(PurpleConnection *gc, void *data);
    static void w_received_im_msg(PurpleAccount* account, char* sender, char* message,
                              PurpleConversation* conv, PurpleMessageFlags flags);
    void received_im_msg(PurpleAccount* account, char* sender, char* message,
                              PurpleConversation* conv, PurpleMessageFlags flags);
    void libpurple();
    static void *start_thread(void* obj);
};

#endif
