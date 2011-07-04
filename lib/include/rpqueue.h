#ifndef RPQUEUE_H_
#define RPQUEUE_H_

#include <pthread.h>
#include <semaphore.h>

#define QUEUE_SIZE 64

template <class obj>
class rpqueue
{
public:
    rpqueue();
    ~rpqueue();
    void add(obj newobj);
    obj get();

private:
    sem_t* empty;
    sem_t* usd;
    sem_t* lock;
    int semval;
    obj rpq[QUEUE_SIZE];
};

#endif
