
#include "rpl_con.h"

rpl_con::rpl_con(rpl_network *net, rpl_storage *store,
    void (*cb)(void *rp, Post *p, int rank), void *rp)
{
    this->reposter = rp;
    this->npCB = cb;
    this->pnet = net;
    this->pstore = store;
}

void rpl_con::stop()
{
    if(running==true){
         running=false;
         pthread_join(m_thread,0);
    }
}   

void rpl_con::go()
{
    if(running==false)
    {
        running = true;
        pthread_create(&m_thread, 0, (&rpl_con::start_thread), this);
    }
}

void *rpl_con::start_thread(void *obj)
{
    reinterpret_cast<rpl_con *>(obj)->consume();
    return NULL;
}

void rpl_con::consume()
{
    // want a timed semwait 
    while(running == true)
    {
      Post *p = pnet->getpost();
      if(p)
      {
        this->pstore->add_post(p);
        npCB(reposter,p,0);
      }
    }
}
