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

class LockStep;

class JabPoster : public slavenet
{
public:
    JabPoster(rpqueue* rq);
    ~JabPoster();

    /* External Thread-safe Interface */
    int GetLinks(Link* links, int num);
    int GetAccounts(Account* accts, int num);
    void RmAccount(Account& acct);
    void AddBonjour(string user);
    void AddJabber(string user, string pass);
    void AddGtalk(string user, string pass);
    void SendPost(Post* post);
    void AddLink(Link& link);
    void RmLink(Link& link);
    std::string GetRepostDir();
    /* Stop and starting libpurple loop thread */
    void Go();
    void Stop();

    static void w_InitUI();
    static void* w_NotifyUserInfo(PurpleConnection *gc, const char *who,
                         PurpleNotifyUserInfo *user_info);
private:
    jabconnections *jabconn; /* Connection handlers */
    rpqueue *in_queue;      /* Post message in queue */
    std::string repostdir;  /* .repost directory */
    GHashTable* resMap;     /* Map of users XMPP resources */
    GMainLoop *loop;        /* LibpurpleLoop glib loop */
    LockStep *lock;         /* Lock for sync between lploop and extern interfaces */

    void ConnectToSignals();
    void InitUI();
    void LibpurpleLoop();
    void PrintSupportedProtocols();
    string GetUniqueIdString();
    static void *StartThread(void* obj);
    void FreeReposterNames(GList* reposters);

    void LockSpinner(void);
    void UnlockSpinner(void);
    static gboolean w_CheckForLock(void *unused);
    void CheckForLock(void);

    /* C Style callbacks and wrappers */
    static gboolean w_LockStep(void *unused);
    static int AuthorizationRequested(PurpleAccount* account, const char* user);
    static void w_ReceivedIm(PurpleAccount* account, char* sender, char* message,
                              PurpleConversation* conv, PurpleMessageFlags flags);
    void ReceivedIm(PurpleAccount* account, char* sender, char* message,
                              PurpleConversation* conv, PurpleMessageFlags flags);
    /* Resource handlers */
    static void w_ResFree(gpointer data);
    void ResFree(gpointer data);
    static gboolean w_RetrieveUserInfo(gpointer data);
    static void w_SignonRetrieveUserInfo(PurpleAccount *account);
    gboolean RetrieveUserInfo(gpointer data);
    void* NotifyUserInfo(PurpleConnection *gc, const char *who,
                         PurpleNotifyUserInfo *user_info);
    GList* ReposterName(PurpleBuddy* pb);
};

#endif
