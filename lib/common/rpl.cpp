
#include "rpl.h"
#include "rpl_network.h"
#include "rpl_storage.h"
#include "rpl_con.h"

#ifndef WIN32
#include <dlfcn.h>
#endif

using namespace std;
#ifdef DEBUG && WIN32
#include "glib.h"
#include "win32/win32dep.h"
void p(const gchar * str)
{
    printf(str);
}
#endif

void rePoster::init() 
{
#ifndef WIN32
    /* We force load libpurple as the chrome plugin loader doesn't 
         do it properly */
    void *handle = dlopen("/usr/lib/libpurple.so",RTLD_LAZY| RTLD_GLOBAL); 
#endif
#if DEBUG && WIN32
    if(AllocConsole()) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        SetConsoleTitle("Debug Console");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 
                FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);  
    }
    g_set_print_handler(p);
#endif

    pnet = new rpl_network();
    rpl_storage::INSTANCE = new rpl_storage();
}

void rePoster::startRepost(){

    pnet->go();

    /* Create storage class here */
    pstore = rpl_storage::get_instance();

    /* Create consumer here */
    pcon = new rpl_con(pnet, pstore, rePoster::cb_wrap, this);
    pcon->go();
}

void rePoster::stopRepost()
{
    pnet->stop();
    pcon->stop();
}

void rePoster::sendPost(Post p)
{
    pnet->post(p);
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
    return pnet->getAccounts();
}

void rePoster::addAccount(Account newaccount)
{
    pnet->addAccount(newaccount);
}   

void rePoster::getInitialPosts(NewPostCB* newPostCB)
{
    int i = 0;
    Post *post[10];
    int rowsReturned = this->pstore->get_post( post, 0, 10 );
    for ( int i = 0; i < rowsReturned; i++ )
    {
        newPostCB->Run(*post[i],0);
    }
}

void rePoster::rmAccount(Account account)
{
    pnet->rmAccount(account);
}

std::vector<Link> rePoster::getLinks()
{
    return pnet->getLinks();
}

void rePoster::addLink(Link newlink)
{
    pnet->addLink(newlink);
}

void rePoster::rmLink(Link link)
{
    pnet->rmLink(link);
}
