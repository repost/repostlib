#ifndef RPQUEUE_H_
#define RPQUEUE_H_

#include <pthread.h>
#include <semaphore.h>
#include "post.h"

#define QUEUE_SIZE 64

class rpqueue
{
public:
    rpqueue();
    ~rpqueue();
    void add(Post *post);
    Post *get();

private:
    sem_t* empty;
    sem_t* usd;
    sem_t* lock;
    Post *postq[QUEUE_SIZE];
};


#endif
