#ifndef JABPOSTER_H_
#define JABPOSTER_H_

#include "rpl.h"
#include "link.h"
#include "slavenetwork.h"
#include "rpqueue.h"
#include "jabconnections.h"

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
    static void w_initUI();
    
private:
    PurpleAccount *acct;
    jabconnections *jabconn;
    rpqueue *in_queue;
    Account pendaccts[MAXACCTS];
    std::string repostdir;

    void connectToSignals();
    void initUI();
    void libpurple();
    void libpurpleDiag();
    static void *start_thread(void* obj);

    /* C Style callbacks and wrappers */
    static int authorization_requested(PurpleAccount* account, const char* user);
    static void w_receivedIm(PurpleAccount* account, char* sender, char* message,
                              PurpleConversation* conv, PurpleMessageFlags flags);
    void receivedIm(PurpleAccount* account, char* sender, char* message,
                              PurpleConversation* conv, PurpleMessageFlags flags);
};

#endif
