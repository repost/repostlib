
#include "rpl.h"
#include "rpl_network.h"
#include "rpl_storage.h"
#include "rpl_con.h"
#include "rpdebug.h"
#include <string>
#include <iostream>
#include "glib.h"

#ifndef WIN32
#include <dlfcn.h>
#endif

using namespace std;
#ifdef WIN32
#include "win32/win32dep.h"
#ifdef DEBUG_ON
void p(const gchar * str)
{
    printf(str);
}
#endif
#endif

void rePoster::init() 
{
#ifdef LINUX
    /* We force load libpurple as the chrome plugin loader doesn't 
         do it properly */
    void *handle = dlopen("/usr/lib/libpurple.so",RTLD_LAZY| RTLD_GLOBAL); 
#endif
#ifdef DEBUG_ON
#ifdef WIN32
    if(AllocConsole()) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        SetConsoleTitle("Debug Console");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 
                FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);  
    }
    g_set_print_handler(p);
#endif
#endif
    InitRepostLogging(GetUserDir());
    LOG(INFO) << "Repost home directory - " << GetUserDir();
    pnet = new rpl_network();
    rpl_storage::INSTANCE = new rpl_storage();
}

void rePoster::startRepost()
{
    LOG(INFO) << "Starting repost...";
    /* Start the networks */
    pnet->go();

    /* lets create bonjour user here */
    Account bacct;
    bacct.set_user("reposter");
    bacct.set_type("Bonjour");
    pnet->addAccount(bacct);

    /* Create storage class here */
    rpl_storage::init(GetUserDir());
    pstore = rpl_storage::get_instance();

    /* Create consumer here */
    pcon = new rpl_con(pnet, pstore, rePoster::cb_wrap, this);
    pcon->go();
}

void rePoster::stopRepost()
{
    LOG(INFO) << "Stopping repost";
    pnet->stop();
    pcon->stop();
}

void rePoster::sendPost(Post p)
{
    LOG(INFO) << "Sending Post UUID " << p.uuid();
    pnet->post(p);
    pstore->add_post(&p);
}

void rePoster::cb(Post *p, int rank)
{
    this->newPostCB->Run(*p, rank);
}

void rePoster::cb_wrap(void *reposter, Post *p, int rank)
{    
    rePoster *rp = (rePoster*)reposter;
    rp->cb(p, rank);
}

std::vector<Account> rePoster::getAccounts()
{
    LOG(INFO) << "Get Accounts";
    return pnet->getAccounts();
}

std::string rePoster::GetUserDir()
{
    string home(PATH_SEPARATOR ".repost");
#ifndef WIN32
    return home.insert(0, g_get_home_dir());
#else
    char *retval = NULL;
    wchar_t utf_16_dir[MAX_PATH + 1];

    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL,
                    SHGFP_TYPE_CURRENT, utf_16_dir))) 
    {
        retval = g_utf16_to_utf8((const gunichar2 *)utf_16_dir, -1, NULL, NULL, NULL);
    }

    return home.insert(0, retval);
#endif
}

void rePoster::addAccount(Account newaccount)
{
    LOG(INFO) << "Add Account " << newaccount.user();
    pnet->addAccount(newaccount);
}   

void rePoster::getInitialPosts(NewPostCB* newPostCB)
{
    LOG(INFO) << "Get Initial Posts";
    int i = 0;
    Post *post[16];
    int rowsReturned = this->pstore->get_post( post, 0, 16 );
    for ( int i = 0; i < rowsReturned; i++ )
    {
        newPostCB->Run(*post[i],0);
    }
}

void rePoster::rmAccount(Account account)
{
    LOG(INFO) << "Remove Account " << account.user();
    pnet->rmAccount(account);
}

std::vector<Link> rePoster::getLinks()
{
    LOG(INFO) << "Get Links";
    return pnet->getLinks();
}

void rePoster::addLink(Link newlink)
{
    LOG(INFO) << "Add Link " << newlink.name();
    pnet->addLink(newlink);
}

void rePoster::rmLink(Link link)
{
    LOG(INFO) << "Remove Link " << link.name();
    pnet->rmLink(link);
}

void rePoster::upboat(string u) 
{
    LOG(INFO) << "Upboating post " << u;
    Post *uppost = NULL;
    pstore->update_metric(u);
    pstore->get_post(&uppost, u);
    if(uppost)
    {
      LOG(DEBUG) << "content " << uppost->content();
      pnet->post(*uppost);
    }
}

void rePoster::downboat(std::string uuid) 
{
    LOG(INFO) << "Downboating post UUID " << uuid;
    pstore->delete_post(uuid);
}

rePoster::~rePoster()
{
    if(pnet)
    {
        pnet->stop();
    }
    if(pcon)
    {
        pcon->stop();
    }
    delete pnet;
    delete pstore;
    delete pcon;
    ShutdownRepostLogging();
}
