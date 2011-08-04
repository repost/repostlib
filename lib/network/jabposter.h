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
class NetworkUiOps;

class JabPoster : public slavenet
{
public:
    JabPoster(rpqueue<Post*>* rq, string repostdir, 
        NetworkUiOps networkuiops);
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

    /* Thread Actions */
    void Go();
    void Stop();

private:
    JabConnections *jabconn_;  /* Connection handlers */
    NetworkUiOps networkuiops_;  /* Network Ui handler */
    rpqueue<Post*> *in_queue_; /* Post message in queue */
    std::string repostdir_;    /* .repost directory */
    GHashTable* resmap_;       /* Map of users XMPP resources */
    GMainLoop *loop_;          /* LibpurpleLoop glib loop */
    GMainContext* con_;        /* Libpurple loop context. Null on everything but linux */
    LockStep *lock_;           /* Lock for sync between lploop and extern interfaces */

    void ConnectToSignals();
    static void w_InitUI();
    void InitUI();
    void PrintSupportedProtocols();
    string GetUniqueIdString();
    void FreeReposterNames(GList* reposters);
    bool PurpleAccount2Repost(PurpleAccount* account, Account* acct);
    bool PurpleBuddy2Link(PurpleBuddy* buddy, Link* link);
    
    /* Thread helpers */
    void LibpurpleLoop();
    static void *StartThread(void* obj);
    void LockSpinner(void);
    void UnlockSpinner(void);
    void CheckForLock(void);
    static GSourceFuncs lockevent;
    static gboolean w_prepare(GSource *source, gint *timeout_);
    static gboolean w_check(GSource *source);
    static gboolean w_dispatch(GSource *source, GSourceFunc callback, gpointer user_data);

    /* C Style callbacks and wrappers */
    static PurpleNotifyUiOps NotifyUiOps;
    static PurpleCoreUiOps CoreUiOps;
    static PurpleAccountUiOps AccountUiOps;
    static int AuthorizationRequested(PurpleAccount* account, const char* user);
    /* Handles receiving IMs */
    static void w_ReceivedIm(PurpleAccount* account, char* sender, char* message,
                              PurpleConversation* conv, PurpleMessageFlags flags);
    void ReceivedIm(PurpleAccount* account, char* sender, char* message,
                              PurpleConversation* conv, PurpleMessageFlags flags);
    /* Buddy already on your list adds you to theirs */
    static void w_NotifyAdded(PurpleAccount *account,const char *remote_user, const char *id,
	                            const char *alias, const char *message);
    void NotifyAdded(PurpleAccount *account,const char *remote_user, const char *id,
	                            const char *alias, const char *message);
    /* Someone not on our list added us to theirs. Prompt to add them */
    static void w_RequestAdd(PurpleAccount *account, const char *remote_user, const char *id,
                                const char *alias, const char *message);
    void RequestAdd(PurpleAccount *account, const char *remote_user, const char *id,
                                const char *alias, const char *message);
    /* Someone not on our list is requesting to add us to their list. */
    static void* w_RequestAuthorize(PurpleAccount *account, const char *remote_user,
                                 const char *id, const char *alias, const char *message,
	                             gboolean on_list, PurpleAccountRequestAuthorizationCb authorize_cb,
	                             PurpleAccountRequestAuthorizationCb deny_cb, void *user_data);
    void* RequestAuthorize(PurpleAccount *account, const char *remote_user,
                                 const char *id, const char *alias, const char *message,
	                             gboolean on_list, PurpleAccountRequestAuthorizationCb authorize_cb,
	                             PurpleAccountRequestAuthorizationCb deny_cb, void *user_data);
    /* Buddy status updates */
    static void w_BuddyStatusChanged(PurpleBuddy *buddy); /* There are more arguments we can use */
    void BuddyStatusChanged(PurpleBuddy *buddy);

    /* XMPP Resource handlers */
    static void w_ResFree(gpointer data);
    void ResFree(gpointer data);
    static gboolean w_RetrieveUserInfo(gpointer data);
    gboolean RetrieveUserInfo(gpointer data);
    static void w_SignonRetrieveUserInfo(PurpleAccount *account);
    static void* w_NotifyUserInfo(PurpleConnection *gc, const char *who,
                         PurpleNotifyUserInfo *user_info);
    void* NotifyUserInfo(PurpleConnection *gc, const char *who,
                         PurpleNotifyUserInfo *user_info);
    GList* ReposterName(PurpleBuddy* pb);
};

#endif
