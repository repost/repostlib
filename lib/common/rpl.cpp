
#include "rpl.h"
#include "rpl_network.h"
#include "rpl_storage.h"
#include "rpl_con.h"
#include <string>
#include <iostream>

#ifndef WIN32
#include <dlfcn.h>
#endif

using namespace std;
#ifdef DEBUG
#ifdef WIN32
#include "glib.h"
#include "win32/win32dep.h"
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
#ifdef DEBUG 
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

    pnet = new rpl_network();
    rpl_storage::INSTANCE = new rpl_storage();
}

void rePoster::startRepost(){

    Account bacct;
    /* lets create bonjour user here */
    bacct.set_user("reposter");
    bacct.set_type("Bonjour");
    pnet->addAccount(bacct);

    pnet->go();

    /* Create storage class here */
    rpl_storage::init(pnet->get_userdir());
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

void rePoster::upboat(string u) {
    Post *uppost = NULL;
    std::cout << "upboated! update metric!" << std::endl;
    pstore->update_metric(u);
    pstore->get_post(&uppost, u);
    if(uppost)
    {
      cout << "uuid " << uppost->uuid() << endl;
      cout << "content " << uppost->content()<< endl;
      pnet->post(*uppost);
    }
}

void rePoster::downboat(std::string uuid) {
    std::cout << "downboated! delete that shit" << std::endl;
    pstore->delete_post(uuid);
}

rePoster::~rePoster()
{
    if(pnet)
        pnet->stop();
    //pstore->stop();
    if(pcon)
        pcon->stop();
    delete pnet;
    delete pstore;
    delete pcon;
}
