#include <signal.h>
#include <string.h>
#include <sstream>
#include "eventloop.h"
#include "defines.h"
#include "jabposter.h"
#include "jabconnections.h"
#include "lockstep.h"
#include "rpdebug.h"
#include "networkuiops.h"

#ifndef WIN32 /* Always last include */
#include <unistd.h>
#else
#include "win32/win32dep.h"
#endif

#ifdef RPTHREAD_SAFE
#define START_THREADSAFE LockSpinner();
#define END_THREADSAFE   UnlockSpinner();
#else
#define START_THREADSAFE do{}while(0);
#define END_THREADSAFE   do{}while(0);
#endif

#define IDENTIFY_STRING "reposter"

#define STATUS_ONLINE   "online"
#define STATUS_OFFLINE  "offline"
#define STATUS_REPOSTER "reposter"

static JabPoster *jabint = NULL;

#if OS_MACOSX
void query_cert_chain(
                        PurpleSslConnection *gsc,
                        const char *hostname, 
                        void* certs, 
                        void (*query_cert_cb)(gboolean trusted, void *userdata), 
                        void *userdata
                    ) 
{
  /* only the jabber service supports this right now */
  query_cert_cb(true, userdata);
}

extern "C" {
    extern gboolean purple_init_ssl_plugin(void);
    extern gboolean purple_init_ssl_cdsa_plugin(void);
}

void init_ssl_plugins()
{
  /* First, initialize our built-in plugins */
  purple_init_ssl_plugin();
  purple_init_ssl_cdsa_plugin();
  PurplePlugin *cdsa_plugin = purple_plugins_find_with_name("CDSA");
  if(cdsa_plugin) 
  {
    gboolean ok = false; 
    purple_plugin_ipc_call(cdsa_plugin, "register_certificate_ui_cb", &ok, query_cert_chain);
  }   
}
#endif

PurpleCoreUiOps JabPoster::CoreUiOps =
{
        NULL,
        NULL,
        &JabPoster::w_InitUI,
        NULL,
        /* padding */
        NULL,
        NULL,
        NULL,
        NULL
};

PurpleNotifyUiOps JabPoster::NotifyUiOps =
{
        NULL, /* pidgin_notify_message, */
        NULL, /* pidgin_notify_email, */
        NULL, /* pidgin_notify_emails, */
        NULL, /* pidgin_notify_formatted, */
        NULL, /* pidgin_notify_searchresults, */
        NULL, /* pidgin_notify_searchresults_new_rows, */
        &JabPoster::w_NotifyUserInfo, /* pidgin_notify_userinfo, */
        NULL, /* pidgin_notify_uri, */
        NULL, /* pidgin_close_notify, */
        NULL,
        NULL,
        NULL,
        NULL 
};

PurpleAccountUiOps JabPoster::AccountUiOps =
{
    &JabPoster::w_NotifyAdded,
    &JabPoster::w_StatusChanged,
    &JabPoster::w_RequestAdd,
    &JabPoster::w_RequestAuthorize,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

GSourceFuncs JabPoster::lockevent =
{
    &JabPoster::w_prepare,
    &JabPoster::w_check,
    &JabPoster::w_dispatch,
    NULL
};

void JabPoster::ConnectToSignals(void)
{
    static int handle;
    purple_signal_connect(purple_conversations_get_handle(), "received-im-msg", &handle,
            PURPLE_CALLBACK(&JabPoster::w_ReceivedIm), NULL);
	purple_signal_connect(purple_blist_get_handle(), "buddy-status-changed", &handle,
				PURPLE_CALLBACK(&JabPoster::w_BuddyStatusChanged), NULL);
	purple_signal_connect(purple_blist_get_handle(), "buddy-idle-changed", &handle,
				PURPLE_CALLBACK(&JabPoster::w_BuddyStatusChanged), NULL);
	purple_signal_connect(purple_blist_get_handle(), "buddy-signed-on", &handle,
				PURPLE_CALLBACK(&JabPoster::w_BuddyStatusChanged), NULL);
	purple_signal_connect(purple_blist_get_handle(), "buddy-signed-off", &handle,
				PURPLE_CALLBACK(&JabPoster::w_BuddyStatusChanged), NULL);
}

void JabPoster::w_InitUI(void)
{
  if( jabint )
  {
    jabint->InitUI();   
  }
}

void JabPoster::InitUI(void)
{ 
    LOG(INFO) << "initialise UI";
#if OS_MACOSX
    init_ssl_plugins();
#endif
    jabconn_ = new JabConnections(networkuiops_);
    purple_connections_set_ui_ops(jabconn_->GetUiOps());
    purple_notify_set_ui_ops(&JabPoster::NotifyUiOps);
    purple_accounts_set_ui_ops(&JabPoster::AccountUiOps);
}

void JabPoster::w_ReceivedIm(PurpleAccount *account, char *sender, char *message,
                              PurpleConversation *conv, PurpleMessageFlags flags)
{
    if(message != NULL)
    {    
        jabint->ReceivedIm(account, sender, message, conv, flags);
    }
}

void JabPoster::ReceivedIm(PurpleAccount *account, char *sender, char *message,
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
        if(post->content().length() == 0)
        {
            LOG(WARNING) << "Post with empty content. Dropping post: " 
							<< post->uuid() << " Sent from: " << sender;
						LOG(DEBUG) << "Message content: " << message;
            delete post;
        }
        else
        {
            jabint->in_queue_->add(post);
        }
    }
}

void JabPoster::w_NotifyAdded(PurpleAccount *account, const char *remote_user, const char *id,
	                            const char *alias, const char *message)
{
    if( jabint )
    {
        jabint->NotifyAdded(account, remote_user, id, alias, message);   
    }
}

void JabPoster::NotifyAdded(PurpleAccount *account, const char *remote_user, const char *id,
	                            const char *alias, const char *message)
{
    Account acct;
    Link link;

    PurpleAccount2Repost(account, &acct);
    link.set_name(remote_user);
    networkuiops_.NotifyAdded(acct, link, message);
}

void JabPoster::w_StatusChanged(PurpleAccount *account, PurpleStatus *status)
{
    if( jabint )
    {
        jabint->StatusChanged(account, status);
    }
}

void JabPoster::StatusChanged(PurpleAccount *account, PurpleStatus *status)
{
    Account acct;
    LOG(DEBUG) << "STATUS CHANGED";
    PurpleAccount2Repost(account, &acct);
    if( purple_status_is_online(status) )
    {
        acct.set_status(STATUS_ONLINE);
    }
    else
    {
        acct.set_status(STATUS_OFFLINE);
    }
    networkuiops_.StatusChanged(acct);
}

void JabPoster::w_RequestAdd(PurpleAccount *account, const char *remote_user, const char *id,
                            const char *alias, const char *message)
{
    if( jabint )
    {
        jabint->RequestAdd(account, remote_user, id, alias, message);   
    }
}

void JabPoster::RequestAdd(PurpleAccount *account, const char *remote_user, const char *id,
                            const char *alias, const char *message)
{
    Account acct;
    Link link;

    PurpleAccount2Repost(account, &acct);
    link.set_name(remote_user);
    networkuiops_.RequestAdd(acct, link, message);

}

void* JabPoster::w_RequestAuthorize(PurpleAccount *account, const char *remote_user,
                             const char *id, const char *alias, const char *message,
                             gboolean on_list, PurpleAccountRequestAuthorizationCb authorize_cb,
                             PurpleAccountRequestAuthorizationCb deny_cb, void *user_data)
{
    if( jabint )
    {
        return jabint->RequestAuthorize(account, remote_user, id, alias, message, on_list, 
                                    authorize_cb, deny_cb, user_data);   
    }
		return NULL;
}

void* JabPoster::RequestAuthorize(PurpleAccount *account, const char *remote_user,
                             const char *id, const char *alias, const char *message,
                             gboolean on_list, PurpleAccountRequestAuthorizationCb authorize_cb,
                             PurpleAccountRequestAuthorizationCb deny_cb, void *user_data)
{
    Account acct;
    Link link;

    PurpleAccount2Repost(account, &acct);
    link.set_name(remote_user);
    if (networkuiops_.RequestAuthorizeAdded(acct, link, message, on_list))
    {
        authorize_cb(NULL);
    }
    else
    {
        deny_cb(NULL);
    }
    return NULL;
}

void JabPoster::w_BuddyStatusChanged(PurpleBuddy *buddy)
{
    if( jabint )
    {
        jabint->BuddyStatusChanged(buddy);  
    }
}

void JabPoster::BuddyStatusChanged(PurpleBuddy *buddy)
{
    Link link;
    PurpleBuddy2Link(buddy, &link);
    networkuiops_.LinkStatusChanged(link);
}

void JabPoster::PrintSupportedProtocols(void)
{
    GList *iter;
    int i;
    iter = purple_plugins_get_protocols();
    LOG(DEBUG) << "Supported Protocols";
    for (i = 0; iter; iter = iter->next) 
    {
      PurplePlugin *plugin = (PurplePlugin *)iter->data;
      PurplePluginInfo *info = plugin->info;
      if (info && info->name) 
      {
        LOG(DEBUG) << "\t" << i++ << " " << info->name;
      }
    }
}

std::string JabPoster::GetRepostDir(void)
{
  return repostdir_;
}

/**
 * Some accounts are just for reposting! Ouuuttrraageoouss!
 * So we need to get the special repost name of such accounts.
 * For xmpp we have resources so we return the reposter resources.
 * 
 * Other account types I got no idea. Probably check status to 
 * see if they reposting etc... That is a TODO
 */
void JabPoster::w_ResFree(gpointer data)
{
    if( jabint )
    {
        jabint->ResFree(data);
    }
}

void JabPoster::ResFree(gpointer data)
{
    GList* resources = (GList*) data;
    for(;resources;resources = g_list_next(resources))
    {
        g_free(resources->data);
    }
    g_list_free(resources);
}

void JabPoster::w_SignonRetrieveUserInfo(PurpleAccount *account)
{
     if( jabint )
    {
        jabint->RetrieveUserInfo(NULL);
    }
}

gboolean JabPoster::w_RetrieveUserInfo(gpointer data)
{                 
    if( jabint )
    {
        return jabint->RetrieveUserInfo(data);
    }
    return true;
}

gboolean JabPoster::RetrieveUserInfo(gpointer data)
{                                      
    PurpleBlistNode * bnode = purple_blist_get_root();
    while(bnode != NULL)
    {
        if(bnode->type == PURPLE_BLIST_BUDDY_NODE)
        {
            PurpleBuddy* pb = PURPLE_BUDDY(bnode);
            PurplePresence* pres = purple_buddy_get_presence(pb);
            const char* name = purple_buddy_get_name(pb);
            if(purple_presence_is_online(pres))
            {
                LOG(DEBUG) << "requesting info for buddy " << name;
                PurpleConnection* conn = purple_account_get_connection(purple_buddy_get_account(pb));
                PurpleNotifyUserInfo *info = purple_notify_user_info_new(); 
                purple_notify_userinfo(conn, name, info, NULL, NULL);
                purple_notify_user_info_destroy(info); 
                serv_get_info(conn, name); 
            }
            else /* signed off remove resources */
            {
                LOG(DEBUG) << "buddy is offline " << name;
                g_hash_table_remove(resmap_, name); 
            }
        }
        bnode = purple_blist_node_next (bnode, false);
    }
    return true;
}                         

void* JabPoster::w_NotifyUserInfo(PurpleConnection *gc, const char *who,
                         PurpleNotifyUserInfo *user_info)
{
    if( jabint )
    {
        return jabint->NotifyUserInfo(gc, who, user_info);
    }
    return NULL;
}

void* JabPoster::NotifyUserInfo(PurpleConnection *gc, const char *who,
                         PurpleNotifyUserInfo *user_info)
{
    PurpleAccount* ac = purple_connection_get_account(gc);
    PurpleBuddy* pb = purple_find_buddy(ac, who);
    PurpleBlistNode* pbln = (PurpleBlistNode*)pb;

    /* Collect new resources */
    GList *resources = NULL;
    GList *l, *info = purple_notify_user_info_get_entries(user_info);
    for (l = info; l; l = l->next) 
    {
        PurpleNotifyUserInfoEntry* user_info_entry = (PurpleNotifyUserInfoEntry *)l->data;
        const char *label = purple_notify_user_info_entry_get_label(user_info_entry);
        const char *value = purple_notify_user_info_entry_get_value(user_info_entry);
        if (label && value)
        {
            if(!strncmp(label,"Resource",sizeof("Resource")))
            {
                LOG(DEBUG) << "Info from buddy " << who << " res " << value;
                char* resname = (char*)g_malloc(strlen(who)+strlen(value)+2); 
                strncpy(resname, who, strlen(who)+1);
                strncat(resname, "/", 1);
                strncat(resname, value, strlen(value));
                resources = g_list_prepend(resources, resname);
            }
        }
    }
        
    /* Replace current resources for user. GHashTable takes care of cleanup */
    if(resources)
    {
        char *who_cpy = (char *) g_malloc(strlen(who));
        strncpy(who_cpy, who, strlen(who));
        g_hash_table_replace(resmap_, (void *)who_cpy, resources);
    }
    return NULL;
}

GList* JabPoster::ReposterName(PurpleBuddy* pb)
{
    GList* reposters = NULL;
    PurpleBlistNode* pbln = (PurpleBlistNode*)pb;
    PurpleAccount* acc = purple_buddy_get_account(pb);
    const char* proto_id = purple_account_get_protocol_id(acc); 
    const char* bname = purple_buddy_get_name(pb);

    if(!strncmp(proto_id, "prpl-jabber", sizeof("prpl-jabber")))
    {
        GList* resources = (GList*)g_hash_table_lookup(resmap_, bname);
        for(resources = g_list_first(resources);resources; resources = g_list_next(resources))
        {
            char* resname = (char*) resources->data;
            if(strstr(resname, IDENTIFY_STRING))
            {
                LOG(DEBUG) << "resource " << resname;
                char* resname_cpy = (char*) g_malloc(strlen(resname)+1);
                strncpy(resname_cpy, resname, strlen(resname)+1);
                reposters = g_list_prepend(reposters, resname_cpy);
            }
        }
    }
    else if(!strncmp(proto_id, "prpl-bonjour", sizeof("prpl-bonjour")))
    {
        LOG(DEBUG) << "prpl-bonjour looking up " << bname;
        if(strstr(purple_buddy_get_name(pb), IDENTIFY_STRING))
        {
            LOG(DEBUG) << "get buddy " << purple_buddy_get_name(pb);
            char* bname = (char*) g_malloc(strlen(purple_buddy_get_name(pb))+1);
            strncpy(bname, purple_buddy_get_name(pb), strlen(purple_buddy_get_name(pb)));
            reposters = g_list_prepend(reposters, bname);
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

void JabPoster::FreeReposterNames(GList* reposters)
{
    GList* rnames = NULL;
    for(rnames = reposters; rnames; rnames = g_list_next(rnames))
    {
        g_free(rnames->data);
    }
    g_list_free(reposters);
}

void JabPoster::SendPost(Post *post)
{
    string strpost;
    PurpleConversation* conv = NULL;
    PurpleBuddy* pb = NULL;
    char* nomarkup = NULL;
    GList* reposters = NULL;
    PurpleBlistNode* bnode = NULL;

    START_THREADSAFE

    if(!post)
    {
        LOG(INFO) << "post is null :'(. All your bases belong to us";
    }
    else
    {
        bnode =  purple_blist_get_root();
        post2xml(&strpost, post);
        while(bnode != NULL)
        {
            if(bnode->type == PURPLE_BLIST_BUDDY_NODE)
            {
                pb = PURPLE_BUDDY(bnode);
                /* Ok we have a buddy now lets find the reposter */
                reposters = ReposterName(pb); 
                if(reposters)
                {
                    GList* rnames = NULL;
                    for(rnames = reposters; rnames; rnames = g_list_next(rnames))
                    {
                        LOG(INFO) << "sending to " << (const char*)rnames->data;
                        conv = purple_conversation_new(PURPLE_CONV_TYPE_IM,
                                purple_buddy_get_account(pb),
                                (const char*) rnames->data);
                        if(conv)
                        {	
                            PurpleMessageFlags flag = (PurpleMessageFlags) (PURPLE_MESSAGE_RAW);
                            nomarkup =  g_markup_escape_text(strpost.c_str(), -1);
                            if(PURPLE_CONV_IM(conv))
                            {
                                LOG(DEBUG) << "send im";
                                purple_conv_im_send_with_flags(PURPLE_CONV_IM(conv), 
                                        nomarkup,flag);
                            }
                            else if(PURPLE_CONV_CHAT(conv))
                            {
                                LOG(DEBUG) << "send chat";
                                purple_conv_chat_send_with_flags(PURPLE_CONV_CHAT(conv), 
                                        nomarkup,flag);
                            }
                            else
                            {
                                LOG(WARNING) << "Other unexpected convo type";
                            }
                            g_free(nomarkup);
                            purple_conversation_destroy(conv); /* have to destroy each convo so we 
                                                                * don't lock to a single resource*/
                        }
                    }
                    /* ok its our job to clean up the glist */
                    this->FreeReposterNames(reposters);
                }
            }
            bnode = purple_blist_node_next (bnode, false);
        }
    }
    END_THREADSAFE
}

bool JabPoster::PurpleAccount2Repost(PurpleAccount* account, Account* acct)
{
    const char* user = NULL;
    const char* type = NULL;

    user = purple_account_get_username(account);
    type = purple_account_get_protocol_name(account);
    if(user && type)
    {
        acct->set_user(user);
        acct->set_type(type);
        if(purple_account_is_connected(account))
        {
            acct->set_status(STATUS_ONLINE);
        }
        else
        {
            acct->set_status(STATUS_OFFLINE);
        }
    }
    else
    {
        return false;
    }
    return true;
}

bool JabPoster::PurpleBuddy2Link(PurpleBuddy* buddy, Link* link)
{
    const char* name = NULL;
    const char* host = NULL;
    PurpleStatus* status = NULL; 
    PurplePresence* pres = NULL;
    PurpleAccount* acct = NULL;
    GList* reposters = NULL;
    
    acct = purple_buddy_get_account(buddy);
    name = purple_buddy_get_name(buddy);
    host = purple_account_get_username(acct);
    pres = purple_buddy_get_presence(buddy);
    if(name && host && pres)
    {
        link->set_name(name);
        link->set_host(host);
        if( !purple_presence_is_online(pres) )
        {
            link->set_status(STATUS_OFFLINE);
        }
        else if((reposters = this->ReposterName(buddy)))
        {
            link->set_status(STATUS_REPOSTER);
        }
        else
        {
            link->set_status(STATUS_ONLINE);
        }
    }
    else
    {
        return false;
    }
    return true;
}

int JabPoster::GetAccounts(Account* accts, int num)
{
    int x = 0;
    const char* user = NULL;
    const char* type = NULL;
    GList* alist = NULL;

    START_THREADSAFE

    alist = purple_accounts_get_all();
    alist = g_list_first(alist);
    while((alist != NULL) && (x < num))
    {
        PurpleAccount* account = (PurpleAccount*) alist->data;
        if(account)
        {
            if(PurpleAccount2Repost(account, &accts[x]))
            {
                x++;
            }
        }
        alist = g_list_next(alist);
    }
    END_THREADSAFE
    return x;
}

void JabPoster::AddLink(Link& link)
{
    GList *l = NULL;
    START_THREADSAFE
    for (l = purple_accounts_get_all_active(); l != NULL; l = l->next) 
    {
        PurpleAccount *account = (PurpleAccount *)l->data;
        if(account && (purple_account_get_username(account) == link.host()))
        {
            purple_account_add_buddy(account, 
                purple_buddy_new(account, link.name().c_str(), NULL));
        }
    }
    END_THREADSAFE
}

void JabPoster::RmLink(Link& link)
{
    int x = 0;
    const char* name = NULL;
    const char* host = NULL;
    PurpleAccount *account = NULL;
    PurpleBlistNode* bnode = NULL;
    
    START_THREADSAFE

    bnode = purple_blist_get_root();
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
    END_THREADSAFE
}

int JabPoster::GetLinks(Link* links, int num)
{
    int x = 0;
    const char* name = NULL;
    const char* host = NULL;
    PurpleStatus* status = NULL; 
    PurplePresence* pres = NULL;
    PurpleBlistNode* bnode = NULL;
    GList* reposters = NULL;
    
    START_THREADSAFE
    
    bnode = purple_blist_get_root();
    while((bnode != NULL) && (x < num))
    {
        if(bnode->type == PURPLE_BLIST_BUDDY_NODE)
        {
            if(PurpleBuddy2Link(PURPLE_BUDDY(bnode), &links[x]))
            {
                x++;
            }
            else
            {
                /* TODO log failure */
            }
        }
        bnode = purple_blist_node_next (bnode, false);
    }
    END_THREADSAFE
    return x;
}

string JabPoster::GetUniqueIdString(void)
{
    time_t now = time( NULL );
    std::stringstream unique_str;
    unique_str << IDENTIFY_STRING;
    unique_str << now%100000;
    return unique_str.str();
}

void JabPoster::AddGtalk(string user, string pass)
{
    PurpleSavedStatus* status = NULL;
    string uniqueid = this->GetUniqueIdString();
    START_THREADSAFE

    /* We need to add reposter postfix so we can find each other */
    size_t slash = user.rfind("/");
    if (slash!=string::npos)
    {
        user.replace(slash + 1, user.length(), uniqueid);
    }
    else
    {
        user.append("/");
        user.append(uniqueid);
    }

    /* Create the account */
    PurpleAccount *jabacct = purple_account_new(user.c_str(), "prpl-jabber");

    /* Get the password for the account */
    purple_account_set_password(jabacct, pass.c_str());

    /* For gtalk account as we have special settings */
    purple_account_set_string(jabacct,"connection_security", "old_ssl");
    purple_account_set_int(jabacct,"port", 443);
    purple_account_set_string(jabacct,"connect_server", "talk.google.com");

    /* It's necessary to enable the account first. */
    purple_accounts_add(jabacct);
    purple_account_set_enabled(jabacct, UI_ID, TRUE);

    status = purple_savedstatus_new(NULL, PURPLE_STATUS_UNAVAILABLE);
    purple_savedstatus_activate(status);
    END_THREADSAFE
}

void JabPoster::AddJabber(string user, string pass)
{
    PurpleSavedStatus *status;
    string uniqueid = this->GetUniqueIdString();
    START_THREADSAFE

    /* We need to add reposter postfix so we can find each other */
    size_t slash = user.rfind("/");
    if (slash!=string::npos)
    {
        user.replace(slash + 1, user.length(), uniqueid);
    }
    else
    {
        user.append("/");
        user.append(uniqueid);
    }
    
    /* Create the account */
    PurpleAccount *jabacct = purple_account_new(user.c_str(), "prpl-jabber");

    /* Get the password for the account */
    purple_account_set_password(jabacct, pass.c_str());

    purple_account_set_bool(jabacct,"opportunistic_tls", TRUE);
    purple_account_set_bool(jabacct,"require_tls", FALSE);

    /* It's necessary to enable the account first. */
    purple_accounts_add(jabacct);
    purple_account_set_enabled(jabacct, UI_ID, TRUE);
    /* Unavailable so we have low priority */
    status = purple_savedstatus_new(NULL, PURPLE_STATUS_UNAVAILABLE);
    purple_savedstatus_activate(status);
    END_THREADSAFE
}

void JabPoster::AddBonjour(string user)
{
    PurpleSavedStatus* status = NULL;
    PurpleAccount* bon = NULL;

    START_THREADSAFE
    /* We need to add reposter postfix so we can find each other */
    bon = purple_account_new(user.c_str(),"prpl-bonjour");
    purple_accounts_add(bon);
    purple_account_set_enabled(bon, UI_ID, TRUE);
    purple_account_connect(bon);
    status = purple_savedstatus_new(NULL, PURPLE_STATUS_AVAILABLE);
    purple_savedstatus_activate(status);
    END_THREADSAFE
}

void JabPoster::RmAccount(Account& acct)
{
    PurpleAccount* pbacct = NULL;
    START_THREADSAFE
    if((acct.type() == "XMPP")||(acct.type() == "Gtalk"))
    {
        pbacct = purple_accounts_find(acct.user().c_str(), "prpl-jabber");
    }
    else if(acct.type() == "Bonjour")
    {
         pbacct = purple_accounts_find(acct.user().c_str(), "prpl-bonjour");
    }
    
    if(pbacct)
    {
        LOG(INFO) << "Deleting " << acct.user();
        purple_accounts_delete(pbacct);
    }
    END_THREADSAFE
}

#ifdef OS_MACOSX
static void ZombieKiller_Signal(int i)
{
    int status;
    pid_t child_pid;

    while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0);
}
#endif

JabPoster::JabPoster(rpqueue<Post*>* rq, string repostdir, NetworkUiOps networkuiops):
    lock_(NULL), in_queue_(rq), repostdir_(repostdir), networkuiops_(networkuiops)
{
    jabint = this;
    resmap_ = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, &JabPoster::w_ResFree);
    
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

    purple_core_set_ui_ops(&JabPoster::CoreUiOps);
    purple_eventloop_set_ui_ops(RepostEventloopUiOps());

    /* set the users directory to live inside the repost settings dir */
    purple_util_set_user_dir(repostdir_.c_str());

    /* Set 
     * Now that all the essential stuff has been set, let's try to init the core. It's
     * necessary to provide a non-NULL name for the current ui to the core. This name
     * is used by stuff that depends on this ui, for example the ui-specific plugins. */
    if(!purple_core_init(UI_ID)) 
    {
        /* Initializing the core failed. Terminate. */
        LOG(FATAL) << "libpurple initialization failed. Dumping core.";
        abort();
    }

    purple_set_blist(purple_blist_new());
    purple_blist_load();
    purple_prefs_load();
    purple_plugins_load_saved(PLUGIN_SAVE_PREF);
    purple_pounces_load();
    this->ConnectToSignals();
#ifdef DEBUG_ON
    PrintSupportedProtocols();
#endif

    g_timeout_add(60000, &JabPoster::w_RetrieveUserInfo, NULL);

#ifdef RPTHREAD_SAFE
    lock_ = new LockStep();
    GSource *lockeventsource = g_source_new(&JabPoster::lockevent, sizeof(GSource));
    g_source_attach(lockeventsource, con_);
#endif
}

JabPoster::~JabPoster()
{
    delete jabconn_;
    uninit_xml();
    g_hash_table_destroy(this->resmap_);

}

void JabPoster::Go()
{
    if(running == false)
    {
        running = true;
        pthread_create(&m_thread, 0, (&JabPoster::StartThread), this);
    }
}

void JabPoster::Stop()
{
    if(running == true)
    {
        running = false;
        this->in_queue_->add(NULL); /* we going down */
        g_main_loop_quit(loop_);
        pthread_join(m_thread,0);
        LOG(DEBUG) << "Jabposter thread joined";
    }
}

void *JabPoster::StartThread(void *obj)
{
    reinterpret_cast<JabPoster *>(obj)->LibpurpleLoop();
    return NULL;
}

void JabPoster::LibpurpleLoop()
{
    con_ = NULL;
#ifdef LINUX
    con_ = g_main_context_new();
#endif
    loop_ = g_main_loop_new(con_, FALSE);
    if(loop_ == NULL)
    {
      LOG(FATAL) << "GLOOP FAIL WE IN DA SHIT";
    }
    g_main_loop_run(loop_);
    LOG(DEBUG) << "Quiting Libpurple core";
    purple_core_quit();
}

void JabPoster::LockSpinner(void)
{
    lock_->LockSpinner();
    lock_->CheckBoss();
}

void JabPoster::UnlockSpinner(void)
{
    lock_->UnlockSpinner();
}

void JabPoster::CheckForLock(void)
{
    lock_->CheckSpinner();
}

gboolean JabPoster::w_prepare(GSource *source, gint *timeout_)
{ 
    if( jabint )
    {
        jabint->CheckForLock();
    }
    LOG(DEBUG) << "Prepare";
    *timeout_ = 100; /* ensure that this idle event isn't blocked by poll */
    return true;
}

gboolean JabPoster::w_check(GSource *source)
{
    LOG(DEBUG) << "Checking";
    return false;
}
gboolean JabPoster::w_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
    LOG(DEBUG) << "Dispatch";
    return true;
}
