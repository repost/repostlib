#ifndef RPLCON_H_
#define RPLCON_H_

#include "rpl.h"
#include "rpl_network.h"
#include "rpl_storage.h"
#include "pthread.h"

class rpl_con 
{
public:
    rpl_con(rpl_network *net, rpl_storage *store);
    ~rpl_con();
    void go();
    void stop();
private:
    rpl_network *pnet_;
    rpl_storage *pstore_;
    bool running_;
    pthread_t m_thread;
    static void *start_thread(void *obj);
    void consume();
};

#endif
