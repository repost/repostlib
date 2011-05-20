#include <signal.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include "win32/win32dep.h"
#endif
#include "eventloop.h"
#include "defines.h"
#include "jabposter.h"
#include "rpqueue.h"
#include "rpl.h"

static jabposter *jabint = NULL;

/*** Conversation uiops ***/
void jab_write_conv(PurpleConversation *conv, const char *who, const char *alias,
            const char *message, PurpleMessageFlags flags, time_t mtime)
{
    const char *name;
    if (alias && *alias)
        name = alias;
    else if (who && *who)
        name = who;
    else
        name = NULL;

    printf("(%s) %s %s: %s\n", purple_conversation_get_name(conv),
            purple_utf8_strftime("(%H:%M:%S)", localtime(&mtime)),
            name, message);
}

static PurpleConversationUiOps jab_conv_uiops = 
{
    NULL,                      /* create_conversation  */
    NULL,                      /* destroy_conversation */
    NULL,                      /* write_chat           */
    NULL,                      /* write_im             */
    jab_write_conv,                      /* write_conv           */
    NULL,                      /* chat_add_users       */
    NULL,                      /* chat_rename_user     */
    NULL,                      /* chat_remove_users    */
    NULL,                      /* chat_update_user     */
    NULL,                      /* present              */
    NULL,                      /* has_focus            */
    NULL,                      /* custom_smiley_add    */
    NULL,                      /* custom_smiley_write  */
    NULL,                      /* custom_smiley_close  */
    NULL,                      /* send_confirm         */
    NULL,
    NULL,
    NULL,
    NULL
};

void jab_ui_init(void)
{
    /**
     * This should initialize the UI components for all the modules. Here we
     * just initialize the UI for conversations.
     */
    purple_conversations_set_ui_ops(&jab_conv_uiops);
}

static PurpleCoreUiOps jab_core_uiops = 
{
    NULL,
    NULL,
    NULL,
    jab_ui_init,

    /* padding */
    NULL,
    NULL,
    NULL,
    NULL
};

void conn_error(PurpleConnection *gc, PurpleConnectionError err, const gchar *desc)
{
    printf("error: %s",desc);
}

void jabposter::w_received_im_msg(PurpleAccount *account, char *sender, char *message,
                              PurpleConversation *conv, PurpleMessageFlags flags)
{
    if(message != NULL)
    {    
        jabint->received_im_msg(account, sender, message, conv, flags);
    }
}

void jabposter::received_im_msg(PurpleAccount *account, char *sender, char *message,
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

void jabposter::connect_to_signals(void)
{
    static int handle;
    purple_signal_connect(purple_connections_get_handle(), "connection-error", &handle,
            PURPLE_CALLBACK(conn_error), NULL);
    purple_signal_connect(purple_conversations_get_handle(), "received-im-msg", &handle,
            PURPLE_CALLBACK(&jabposter::w_received_im_msg), NULL);
    purple_signal_connect(purple_accounts_get_handle(), "account-authorization-requested", &handle,
            PURPLE_CALLBACK(&jabposter::authorization_requested), NULL);


}

void jabposter::sendpost(Post *post)
{
    string strpost;
    PurpleConversation* conv = NULL;
    PurpleBlistNode * bnode = purple_blist_get_root();
    char* nomarkup = NULL;

    if(!post)
    {
        /* TODO warn about null post */
        return;
    }

    post2xml(&strpost, post);
    while(bnode != NULL){
        if(bnode->type == PURPLE_BLIST_BUDDY_NODE)
        {
            conv = purple_conversation_new(PURPLE_CONV_TYPE_IM,
                    purple_buddy_get_account(PURPLE_BUDDY(bnode)),
                    purple_buddy_get_name(PURPLE_BUDDY(bnode)));
            if(conv)
            {
                nomarkup =  g_markup_escape_text(strpost.c_str(), -1);
                purple_conv_im_send_with_flags(PURPLE_CONV_IM(conv), 
                        nomarkup,PURPLE_MESSAGE_RAW);
                purple_conversation_destroy(conv);
                g_free(nomarkup);
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
#elif OS_MACOSX
    /* Libpurple's async DNS lookup tends to create zombies. */
    {
      struct sigaction act;
      
      act.sa_handler = ZombieKiller_Signal;		
      //Send for terminated but not stopped children
      act.sa_flags = SA_NOCLDWAIT;

      sigaction(SIGCHLD, &act, NULL);
    }
#endif

    /* We do not want any debugging for now to keep the noise to a minimum. */
#ifdef LIBPURPLE_DEBUG
    purple_debug_set_enabled(TRUE);
#else
    purple_debug_set_enabled(FALSE);
#endif
    /* Set the core-uiops, which is used to
     *     - initialize the ui specific preferences.
     *     - initialize the debug ui.
     *     - initialize the ui components for all the modules.
     *     - uninitialize the ui components for all the modules when the core terminates.
     */
    purple_core_set_ui_ops(&jab_core_uiops);

    /* Set the uiops for the eventloop. If your client is glib-based, you can safely
     * copy this verbatim. */
    purple_eventloop_set_ui_ops(repost_purple_eventloop_get_ui_ops());

    /* set the users directory to live inside the repost settings dir */
    // TODO
    purple_util_set_user_dir("/Users/andrewhankins/.repost/");
	
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

    /* Create and load the buddylist. */
    purple_set_blist(purple_blist_new());
    purple_blist_load();

    /* Load the preferences. */
    purple_prefs_load();

    /* Load the desired plugins. The client should save the list of loaded plugins in
     * the preferences using purple_plugins_save_loaded(PLUGIN_SAVE_PREF) */
    purple_plugins_load_saved(PLUGIN_SAVE_PREF);

    /* Load the pounces. */
    purple_pounces_load();

    /* Now, to connect the account(s), create a status and activate it. */
    connect_to_signals();

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
/* 
 Seems as though g_main_loops don't play nicely so we use the
 native loops as they do in adium
 */
#ifdef OS_MACOSX
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
#else
    GMainContext *con = g_main_context_new();
    GMainLoop *loop = g_main_loop_new(con, FALSE);
#endif
    if(loop == NULL)
    {
      printf("GLOOP FAIL WE IN DA SHIT\n");
    }
    g_main_loop_run(loop);
}


