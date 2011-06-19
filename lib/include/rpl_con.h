#ifndef RPLCON_H_
#define RPLCON_H_

#include "rpl.h"
#include "rpl_network.h"
#include "rpl_storage.h"
#include "pthread.h"

class rpl_con 
{
public:
    rpl_con(rpl_network *net, rpl_storage *store, 
        void (*cb)(void *rp, Post *p, int rank), void *rp);
    ~rpl_con();
    void go();
    void stop();
private:
    rpl_network *pnet;
    rpl_storage *pstore;
    void (*npCB)(void *rp, Post *p, int rank);
    void *reposter;
    bool running;
    pthread_t m_thread;
    static void *start_thread(void *obj);
    void consume();
};

#endif
