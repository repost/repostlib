
#include "rpl.h"
#include "rpl_network.h"
#include "rpl_storage.h"
#include "rpl_con.h"
#ifndef WIN32
#include <dlfcn.h>
#endif
using namespace std;

void rePoster::init() {
    pnet = new rpl_network();
}

void rePoster::startRepost(){

#ifndef WIN32
/* We force load libpurple as the chrome plugin loader doesn't 
    do it properly */
    void *handle = dlopen("/usr/lib/libpurple.so",RTLD_LAZY| RTLD_GLOBAL); 
#endif
    pnet->go();

    /* Create storage class here */
    pstore = NULL;
    
    /* Create consumer here */
    pcon = new rpl_con(pnet, pstore, rePoster::cb_wrap, this);
    pcon->go();
}

void rePoster::stopRepost(){
    pnet->stop();
    pcon->stop();
}

Post rePoster::newPost(){
    
    Post p;
    return p;

}

void rePoster::sendPost(Post p){

    pnet->post(p);

}


void rePoster::cb(Post *p){

    this->newPostCB->Run(*p);

}

void rePoster::cb_wrap(void *reposter, Post *p){
    
    rePoster *rp = (rePoster*)reposter;
    rp->cb(p);

}

void rePoster::addAccount(string user, string pass, string network){

    if( network == "jabber" )
    {
        pnet->add_jab(user, pass, "");
    }
    else if( network == "bonjour")
    {
        pnet->add_bon(user);
    }
    else
    {
    }
}   


