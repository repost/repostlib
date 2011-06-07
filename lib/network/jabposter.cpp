#include <signal.h>
#include <string.h>
#include "eventloop.h"
#include "defines.h"
#include "jabposter.h"
#include "jabconnections.h"
#include "rpqueue.h"
#include "rpl.h"

#ifndef WIN32 /* Always last include */
#include <unistd.h>
#else
#include "win32/win32dep.h"
#endif

static jabposter *jabint = NULL;

static PurpleCoreUiOps jab_core_uiops = 
{
    NULL,
    NULL,
    NULL,
    &jabposter::w_initUI,
    /* padding */
    NULL,
    NULL,
    NULL,
    NULL
};

static void* jabposter_notify_userinfo(PurpleConnection *gc, const char *who,PurpleNotifyUserInfo *user_info);
static PurpleNotifyUiOps jabposterNotifyUiOps =
{
    NULL, /* pidgin_notify_message, */
    NULL, /* pidgin_notify_email, */
    NULL, /* pidgin_notify_emails, */
    NULL, /* pidgin_notify_formatted, */
    NULL, /* pidgin_notify_searchresults, */
    NULL, /* pidgin_notify_searchresults_new_rows, */
    jabposter_notify_userinfo, /* pidgin_notify_userinfo, */
    NULL, /* pidgin_notify_uri, */
    NULL, /* pidgin_close_notify, */
    NULL,
    NULL,
    NULL,
    NULL 
};
void jabposter::w_initUI(void)
{
  if( jabint )
  {
    jabint->initUI();   
  }
}

void jabposter::initUI(void)
{ 
    printf("initialise UI\n");
    this->jabconn = new jabconnections();
    purple_connections_set_ui_ops(this->jabconn->getUiOps());
}

void jabposter::connectToSignals(void)
{
    static int handle;
    purple_signal_connect(purple_conversations_get_handle(), "received-im-msg", &handle,
            PURPLE_CALLBACK(&jabposter::w_receivedIm), NULL);
    purple_signal_connect(purple_accounts_get_handle(), "account-authorization-requested", &handle,
            PURPLE_CALLBACK(&jabposter::authorization_requested), NULL);
}

void jabposter::w_receivedIm(PurpleAccount *account, char *sender, char *message,
                              PurpleConversation *conv, PurpleMessageFlags flags)
{
    if(message != NULL)
    {    
        jabint->receivedIm(account, sender, message, conv, flags);
    }
}

void jabposter::receivedIm(PurpleAccount *account, char *sender, char *message,
                              PurpleConversation *conv, PurpleMessageFlags flags)
{
    char* unescaped = NULL;
    string content;

    /* find and replace <BR> with <br> so we play nicely with the xml parser*/
    char *pos = strstr(message,"<BR>");
    while(pos != NULL) {
        pos[1] = 'b';
        pos[2] = 'r';
        pos = strstr(message,"<BR>");
    }
    unescaped =  purple_unescape_html(message);
    if(unescaped)
    {
        content.assign(unescaped);
#ifdef LIBPURPLE_WORKAROUND
        content.erase(0,sizeof("<body>")-1); /*remove the body tag. -1 for null char size of counts */
        content.erase(content.length()-sizeof("</body>")+1,sizeof("</body>")-1);
#endif 
        g_free(unescaped);
        Post *post = new Post();
        xml2post(&content, post);
        jabint->in_queue->add(post);
    }
}

int jabposter::authorization_requested(PurpleAccount *account, const char *user)
{
    purple_account_add_buddy(account, purple_buddy_new(account,user,NULL));
    return 1;
} 


void jabposter::libpurpleDiag()
{
    GList *iter;
    int i;
    iter = purple_plugins_get_protocols();
    printf("Supported Protocols\n");
    for (i = 0; iter; iter = iter->next) {
      PurplePlugin *plugin = (PurplePlugin *)iter->data;
      PurplePluginInfo *info = plugin->info;
      if (info && info->name) {
        printf("\t%d: %s\n", i++, info->name);
      }
    }
}

std::string jabposter::get_repostdir()
{
  return repostdir;
}

void retrieveUserInfo(PurpleConnection *conn, const char *name)                        
 {                                                                                                  
     PurpleNotifyUserInfo *info = purple_notify_user_info_new();                                    
     purple_notify_userinfo(conn, name, info, NULL, NULL);                               
     purple_notify_user_info_destroy(info);                                                         
     serv_get_info(conn, name);                                                                     
 }                         


/**
 * Some accounts are just for reposting! Ouuuttrraageoouss!
 * So we need to get the special repost name of such accounts.
 * For xmpp we have resources so we return the reposter resources.
 * 
 * Other account types I got no idea. Probably check status to 
 * see if they reposting etc... That is a TODO
 */
GList* jabposter::reposterName(PurpleBuddy* pb)
{
    GList* reposters = NULL;
    PurpleBlistNode* pbln = (PurpleBlistNode*)pb;
    PurpleAccount* acc = purple_buddy_get_account(pb);
    const char* proto_id = purple_account_get_protocol_id(acc); 
    if(!strncmp(proto_id, "prpl-jabber", sizeof("prpl-jabber")))
    {
        GList* res = (GList*) purple_blist_node_get_string(pbln, "resources");
        for(;res; res = g_list_next(res))
        {
            const char* name = "reposter"; //((JabberBuddyResource*)res)->name;
            if(!strstr(name,"reposter"))
            {
                /* we should copy incase purple pulls the rug out from under us */
                char* resname = (char*) g_malloc(strlen(name)+1);
                strncpy(resname, name, strlen(name));
                reposters = g_list_prepend(reposters, resname);
            }
        }
    }
    else /* TODO time being lets just assume that all other are reposters */
    {
        char* bname = (char*) g_malloc(strlen(purple_buddy_get_name(pb))+1);
        strncpy(bname, purple_buddy_get_name(pb), strlen(purple_buddy_get_name(pb)));
        reposters = g_list_prepend(reposters, bname);
    }
    return reposters;
}

void jabposter::sendpost(Post *post)
{
    string strpost;
    PurpleConversation* conv = NULL;
    PurpleBuddy* pb = NULL;
    char* nomarkup = NULL;
    GList* reposters = NULL;
    PurpleBlistNode * bnode = purple_blist_get_root();

    if(!post)
    {
        printf("post is null :'(. All your bases belong to us\n");
        return;
    }

    post2xml(&strpost, post);
    while(bnode != NULL)
    {
        if(bnode->type == PURPLE_BLIST_BUDDY_NODE)
        {
            pb = PURPLE_BUDDY(bnode);
            /* Ok we have a buddy now lets find the reposter */
            reposters = reposterName(pb); 
            if(reposters)
            {
                GList* rnames = NULL;
                for(rnames = reposters; rnames; rnames = g_list_next(rnames))
                {
                    conv = purple_conversation_new(PURPLE_CONV_TYPE_IM,
                            purple_buddy_get_account(pb),
                            (const char*) rnames);
                    if(conv)
                    {	
                        PurpleMessageFlags flag = (PurpleMessageFlags) (PURPLE_MESSAGE_RAW);
                        nomarkup =  g_markup_escape_text(strpost.c_str(), -1);
                        if(PURPLE_CONV_IM(conv))
                        {
                            printf("send im\n");
                            purple_conv_im_send_with_flags(PURPLE_CONV_IM(conv), 
                                    nomarkup,flag);
                        }
                        else if(PURPLE_CONV_CHAT(conv))
                        {
                            printf("send chat\n");
                            purple_conv_chat_send_with_flags(PURPLE_CONV_CHAT(conv), 
                                    nomarkup,flag);
                        }
                        else
                        {
                            printf("Other unexpected convo type\n");
                        }
                        g_free(nomarkup);
                    }
                }
                /* ok its our job to clean up the glist */
                for(rnames = reposters; rnames; rnames = g_list_next(rnames))
                    g_free(rnames);
                g_list_free(reposters);
            }
        }
        bnode = purple_blist_node_next (bnode, false);
    }
}

int jabposter::getaccounts(Account* accts, int num)
{
    int x = 0;
    const char* user = NULL;
    const char* type = NULL;
    GList *alist = purple_accounts_get_all();
    alist = g_list_first(alist);

    while((alist != NULL) && (x < num))
    {
        PurpleAccount* account = (PurpleAccount*) alist->data;
        if(account)
        {
            user = purple_account_get_username(account);
            type = purple_account_get_protocol_name(account);
            if(user && type)
            {
                accts[x].set_user(user);
                accts[x].set_type(type);
                x++;
            }
        }
        alist = g_list_next(alist);
    }
    return x;
}

void jabposter::addlink(Link& link)
{
    GList *l;
    
    for (l = purple_accounts_get_all_active(); l != NULL; l = l->next) 
    {
        PurpleAccount *account = (PurpleAccount *)l->data;
        if(account && (purple_account_get_username(account) == link.host()))
        {
            purple_account_add_buddy(account, 
                purple_buddy_new(account,link.name().c_str(),NULL));
        }
    }
}

void jabposter::rmlink(Link& link)
{
    int x = 0;
    const char* name = NULL;
    const char* host = NULL;
    PurpleAccount *account = NULL;
    PurpleBlistNode * bnode = purple_blist_get_root();

    while(bnode != NULL)
    {
        if(bnode->type == PURPLE_BLIST_BUDDY_NODE)
        {
            name = purple_buddy_get_name(PURPLE_BUDDY(bnode));
            account = purple_buddy_get_account(PURPLE_BUDDY(bnode));
            host = purple_account_get_username(account);
            if((name == link.name()) ) /* && (host == link.host())) */
            {
                purple_account_remove_buddy(account, PURPLE_BUDDY(bnode), 
                            purple_buddy_get_group(PURPLE_BUDDY(bnode)));
            }
        }
        bnode = purple_blist_node_next (bnode, false);
    }
}

int jabposter::getlinks(Link* links, int num)
{
    int x = 0;
    const char* name = NULL;
    const char* host = NULL;
    PurpleBlistNode * bnode = purple_blist_get_root();

    while((bnode != NULL) && (x < num))
    {
        if(bnode->type == PURPLE_BLIST_BUDDY_NODE)
        {
            name = purple_buddy_get_name(PURPLE_BUDDY(bnode));
            host = purple_account_get_username(
                purple_buddy_get_account(PURPLE_BUDDY(bnode)));
            if(name && host)
            {
                links[x].set_name(name);
                links[x].set_host(host);
                x++;
            }
            else
            {
                /* TODO log failure */
            }
        }
        bnode = purple_blist_node_next (bnode, false);
    }
    return x;
}

void jabposter::addJabber(string user, string pass)
{
    PurpleSavedStatus *status;
    /* Create the account */
    PurpleAccount *jabacct = purple_account_new(user.c_str(), "prpl-jabber");

    /* Get the password for the account */
    purple_account_set_password(jabacct, pass.c_str());

    purple_account_set_bool(jabacct,"require_tls",FALSE);

    /* It's necessary to enable the account first. */
    purple_accounts_add(jabacct);
    purple_account_set_enabled(jabacct, UI_ID, TRUE);

    status = purple_savedstatus_new(NULL, PURPLE_STATUS_AVAILABLE);
    purple_savedstatus_activate(status);
}

void jabposter::addBonjour(string user)
{
    PurpleSavedStatus *status;
    PurpleAccount *bon = purple_account_new(user.c_str(),"prpl-bonjour");
    purple_accounts_add(bon);
    purple_account_set_enabled(bon, UI_ID, TRUE);
    purple_account_connect(bon);

    status = purple_savedstatus_new(NULL, PURPLE_STATUS_AVAILABLE);
    purple_savedstatus_activate(status);
}

#ifdef OS_MACOSX
static void ZombieKiller_Signal(int i)
{
    int status;
    pid_t child_pid;

    while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0);
}
#endif

jabposter::jabposter(rpqueue* rq)
{
    jabint = this;
    in_queue = rq;
#ifdef LINUX 
    /* libpurple's built-in DNS resolution forks processes to perform
     * blocking lookups without blocking the main process.  It does not
     * handle SIGCHLD itself, so if the UI does not you quickly get an army
     * of zombie subprocesses marching around.
     */
    signal(SIGCHLD, SIG_IGN);
#elif WIN32
    const char *plugindir = wpurple_install_dir();
    SetDllDirectory(plugindir);
#elif OS_MACOSX
    /* Libpurple's async DNS lookup tends to create zombies. */
    {
      struct sigaction act;
      
      act.sa_handler = ZombieKiller_Signal;		
      /* Send for terminated but not stopped children */
      act.sa_flags = SA_NOCLDWAIT;

      sigaction(SIGCHLD, &act, NULL);
    }
#endif

#ifdef LIBPURPLE_DEBUG
    purple_debug_set_enabled(TRUE);
#else
    purple_debug_set_enabled(FALSE);
#endif

    purple_core_set_ui_ops(&jab_core_uiops);
    purple_eventloop_set_ui_ops(repost_purple_eventloop_get_ui_ops());

    /* set the users directory to live inside the repost settings dir */
    repostdir.assign(purple_home_dir());
    repostdir.append("/.repost");
    purple_util_set_user_dir(repostdir.c_str());

    /* Set 
     * Now that all the essential stuff has been set, let's try to init the core. It's
     * necessary to provide a non-NULL name for the current ui to the core. This name
     * is used by stuff that depends on this ui, for example the ui-specific plugins. */
    if (!purple_core_init(UI_ID)) {
        /* Initializing the core failed. Terminate. */
        fprintf(stderr,
                "libpurple initialization failed. Dumping core.\n"
                "Please report this!\n");
        abort();
    }

    purple_set_blist(purple_blist_new());
    purple_blist_load();
    purple_prefs_load();
    purple_plugins_load_saved(PLUGIN_SAVE_PREF);
    purple_pounces_load();
#ifdef DEBUG
    libpurpleDiag();
#endif
    this->initUI();
    this->connectToSignals();
    purple_notify_set_ui_ops(&jabposterNotifyUiOps);
}

jabposter::~jabposter()
{
    uninit_xml();
}

void jabposter::go(){
    
    if(running == false)
    {
        running = true;
        pthread_create(&m_thread, 0, (&jabposter::start_thread), this);
    }
}

void *jabposter::start_thread(void *obj)
{
    reinterpret_cast<jabposter *>(obj)->libpurple();
    return NULL;
}

void jabposter::libpurple()
{
    GMainContext *con = NULL;
#ifdef LINUX
    con = g_main_context_new();
#endif
    GMainLoop *loop = g_main_loop_new(con, FALSE);
    if(loop == NULL)
    {
      printf("GLOOP FAIL WE IN DA SHIT\n");
    }
    g_main_loop_run(loop);
}


