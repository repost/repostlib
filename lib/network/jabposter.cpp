#include <signal.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include "win32/win32dep.h"
#endif
#include "defines.h"
#include "jabposter.h"
#include "rpqueue.h"
#include "rpl.h"

/**
 * The following eventloop functions are used in both pidgin and purple-text. If your
 * application uses glib mainloop, you can safely use this verbatim.
 */
#define PURPLE_GLIB_READ_COND  (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define PURPLE_GLIB_WRITE_COND (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL)

jabposter *jabint = NULL;

typedef struct _PurpleGLibIOClosure {
	PurpleInputFunction function;
	guint result;
	gpointer data;
} PurpleGLibIOClosure;

static void purple_glib_io_destroy(gpointer data)
{
	g_free(data);
}

static gboolean purple_glib_io_invoke(GIOChannel *source, GIOCondition condition, gpointer data)
{
	PurpleGLibIOClosure *closure = (PurpleGLibIOClosure *) data;
	int purple_cond = 0;

	if (condition & PURPLE_GLIB_READ_COND)
		purple_cond |= PURPLE_INPUT_READ;
	if (condition & PURPLE_GLIB_WRITE_COND)
		purple_cond |= PURPLE_INPUT_WRITE;

	closure->function(closure->data, g_io_channel_unix_get_fd(source),
			  (PurpleInputCondition)purple_cond);

	return TRUE;
}

static guint glib_input_add(gint fd, PurpleInputCondition condition, PurpleInputFunction function,
							   gpointer data)
{
	PurpleGLibIOClosure *closure = g_new0(PurpleGLibIOClosure, 1);
	GIOChannel *channel;
	int cond = 0;

	closure->function = function;
	closure->data = data;

	if (condition & PURPLE_INPUT_READ)
		cond |= PURPLE_GLIB_READ_COND;
	if (condition & PURPLE_INPUT_WRITE)
		cond |= PURPLE_GLIB_WRITE_COND;

#if defined _WIN32 && !defined WINPIDGIN_USE_GLIB_IO_CHANNEL
	channel = wpurple_g_io_channel_win32_new_socket(fd);
#else
	channel = g_io_channel_unix_new(fd);
#endif
	closure->result = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, (GIOCondition)cond,
					      purple_glib_io_invoke, closure, purple_glib_io_destroy);

	g_io_channel_unref(channel);
	return closure->result;
}

static PurpleEventLoopUiOps glib_eventloops = 
{
	g_timeout_add,
	g_source_remove,
	glib_input_add,
	g_source_remove,
	NULL,
#if GLIB_CHECK_VERSION(2,14,0)
	g_timeout_add_seconds,
#else
	NULL,
#endif

	/* padding */
	NULL,
	NULL,
	NULL
};
/*** End of the eventloop functions. ***/

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
 	if(message != NULL)
	{	
            /* find and replace <BR> with <br> */
            char *pos = strstr(message,"<BR>");
            while( pos != NULL) {
                  pos[1] = 'b';
                  pos[2] = 'r';
                  pos = strstr(message,"<BR>");
            }
            char *unescaped =  purple_unescape_html(message);
            string content(unescaped);
            content.erase(0,sizeof("<body>")-1); /*remove the body tag. -1 for null char size of counts */
            content.erase(content.length()-sizeof("</body>")+1,sizeof("</body>")-1);
/*need a free here */
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
    static string strpost;
    post2xml(&strpost, post);
    PurpleConversation* conv;
    PurpleBlistNode * bnode = purple_blist_get_root();
    while(bnode != NULL){
        if(bnode->type == PURPLE_BLIST_BUDDY_NODE){
            conv = purple_conversation_new(PURPLE_CONV_TYPE_IM,
                    purple_buddy_get_account(PURPLE_BUDDY(bnode)),
                    purple_buddy_get_name(PURPLE_BUDDY(bnode)));
            char *nomarkup =  g_markup_escape_text(strpost.c_str(), -1);
            purple_conv_im_send_with_flags(PURPLE_CONV_IM(conv), nomarkup,PURPLE_MESSAGE_RAW);
            purple_conversation_destroy(conv);
            g_free(nomarkup);
        }
        bnode = purple_blist_node_next (bnode, false);
    }
}

void jabposter::addlink(Link &link)
{
#if 0
    GList *l;
    for (l = purple_accounts_get_all_active(); l != NULL; l = l->next) {
        PurpleAccount *account = (PurpleAccount *)l->data;
        purple_account_add_buddy(account, purple_buddy_new(account,link.get_name().c_str(),NULL));
    }
#endif
}


int jabposter::getlinks(Link* links, int num)
{
    int x = 0;
    PurpleBlistNode * bnode = purple_blist_get_root();
    while(bnode != NULL && x < num){
        if(bnode->type == PURPLE_BLIST_BUDDY_NODE){
            links[x].set_name(purple_buddy_get_name(PURPLE_BUDDY(bnode)));
            links[x].set_host(purple_buddy_get_account(PURPLE_BUDDY(bnode))->username);
        }
        bnode = purple_blist_node_next (bnode, false);
    }
    return x;
}

void jabposter::add_jab(string user, string pass, string port)
{
    jab_acct ja;
    ja.user = user;
    ja.pass = pass;
    ja.port = port;
    jabaccts[num_jab_accts++] = ja;
}

void jabposter::add_jabacct(jab_acct ja)
{
	PurpleSavedStatus *status;
	/* Create the account */
	PurpleAccount *jabacct = purple_account_new(ja.user.c_str(), "prpl-jabber");

	/* Get the password for the account */
	purple_account_set_password(jabacct, ja.pass.c_str());

	purple_account_set_bool(jabacct,"require_tls",FALSE);
	//purple_account_set_string(acct, "bosh_url","https://j.sideramota.com/jabber");

	/* It's necessary to enable the account first. */
	purple_accounts_add(jabacct);
	purple_account_set_enabled(jabacct, UI_ID, TRUE);

        status = purple_savedstatus_new(NULL, PURPLE_STATUS_AVAILABLE);
        purple_savedstatus_activate(status);
}


void jabposter::add_bon(string user)
{
    bon_acct ba;
    ba.user = user;         
   bonaccts[num_bon_accts++] = ba;
}


void jabposter::add_bonacct(bon_acct ba)
{
	PurpleSavedStatus *status;
        PurpleAccount *bon = purple_account_new(ba.user.c_str(),"prpl-bonjour");
        purple_accounts_add(bon);
	purple_account_set_enabled(bon, UI_ID, TRUE);

        status = purple_savedstatus_new(NULL, PURPLE_STATUS_AVAILABLE);
        purple_savedstatus_activate(status);
}

jabposter::jabposter(rpqueue *i)
{
    num_bon_accts = 0;
    num_jab_accts = 0;
    jabint = this;
    in_queue = i;
}

jabposter::~jabposter()
{
    uninit_xml();
}

void jabposter::go(){
    
    if(running==false)
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

#ifndef _WIN32
	/* libpurple's built-in DNS resolution forks processes to perform
	 * blocking lookups without blocking the main process.  It does not
	 * handle SIGCHLD itself, so if the UI does not you quickly get an army
	 * of zombie subprocesses marching around.
	 */
	signal(SIGCHLD, SIG_IGN);
#endif
	/* Set a custom user directory (optional) */
    /* Removing for time being to see what effect on plugin it has
	purple_util_set_user_dir(CUSTOM_USER_DIRECTORY);*/

	/* We do not want any debugging for now to keep the noise to a minimum. */
	purple_debug_set_enabled(FALSE);

	/* Set the core-uiops, which is used to
	 * 	- initialize the ui specific preferences.
	 * 	- initialize the debug ui.
	 * 	- initialize the ui components for all the modules.
	 * 	- uninitialize the ui components for all the modules when the core terminates.
	 */
	purple_core_set_ui_ops(&jab_core_uiops);

	/* Set the uiops for the eventloop. If your client is glib-based, you can safely
	 * copy this verbatim. */
	purple_eventloop_set_ui_ops(&glib_eventloops);

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
        
        int x;
        for(x=0;x<num_bon_accts;x++)
            add_bonacct(bonaccts[x]);

        for(x=0;x<num_jab_accts;x++)
            add_jabacct(jabaccts[x]);

    GMainContext *con = g_main_context_new();
	GMainLoop *loop = g_main_loop_new(con, FALSE);
	g_main_loop_run(loop);
}


