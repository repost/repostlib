
#include "rpl.h"
#include "rpl_network.h"
#include "rpl_storage.h"
#include "rpl_con.h"

#ifndef WIN32
#include <dlfcn.h>
#endif

using namespace std;

void rePoster::init() 
{
#ifndef WIN32
    /* We force load libpurple as the chrome plugin loader doesn't 
       do it properly */
    void *handle = dlopen("/usr/lib/libpurple.so",RTLD_LAZY| RTLD_GLOBAL); 
#endif

    pnet = new rpl_network();
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

void rePoster::cb(Post *p)
{
    this->newPostCB->Run(*p);
}

void rePoster::cb_wrap(void *reposter, Post *p)
{    
    rePoster *rp = (rePoster*)reposter;
    rp->cb(p);
}

std::vector<Account> rePoster::getAccounts()
{
    return pnet->getAccounts();
}

void rePoster::addAccount(Account newaccount)
{
    pnet->addAccount(newaccount);
}   

void rePoster::getInitialPosts(NewPostCB *newPostCB)
{
    int i = 0;
    Post *post[10];
    int rowsReturned = this->pstore->get_post( post, 0, 10 );
    for ( int i = 0; i < rowsReturned; i++ )
    {
        this->newPostCB->Run(*post[i]);
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

void rePoster::addLink(Link newlink, Account acct)
{
    pnet->addLink(newlink,acct);
}

void rePoster::rmLink(Link link)
{
    pnet->rmLink(link);
}
