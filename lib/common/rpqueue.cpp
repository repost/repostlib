
#include "rpl.h"
#include "rpqueue.h"
#include <string.h>

#ifdef OS_MACOSX
#define NAMED_SEMAPHORE
#include <time.h>
#endif

template <class obj>
rpqueue<obj>::rpqueue()
{
#ifdef NAMED_SEMAPHORE
    char name[32];
    long now;
    now = (long) time(0);
    snprintf(name, 32, "/%s-%10d-%10ld", "lock", getpid(), now);
    this->lock = sem_open(name, O_CREAT, 0, 1);
    snprintf(name, 32, "/%s-%10d-%10ld", "usd", getpid(), now);
    this->usd = sem_open(name, O_CREAT, 0, 0);
    snprintf(name, 32, "/%s-%10d-%10ld", "empty", getpid(), now);
    this->empty = sem_open(name, O_CREAT, 0, QUEUE_SIZE);
#else
    this->empty =  new sem_t;
    this->lock =  new sem_t;
    this->usd =  new sem_t;
    sem_init(this->empty, 0, QUEUE_SIZE);
    sem_init(this->lock, 0, 1);
    sem_init(this->usd, 0, 0);
#endif
    this->semval = 0;
}  

template <class obj>
rpqueue<obj>::~rpqueue()
{
#ifdef NAMED_SEMAPHORE
    sem_close(empty);
    sem_close(usd);
    sem_close(lock);
#else
    sem_destroy(empty);
    sem_destroy(usd);
    sem_destroy(lock);
#endif
}

template <class obj>
void rpqueue<obj>::add(obj newobj)
{
    sem_wait(empty);
    sem_wait(lock);
    rpq[this->semval] = newobj;
    this->semval += 1;
    sem_post(usd);
    sem_post(lock);
}

template <class obj>
obj rpqueue<obj>::get()
{
    obj ret;
    sem_wait(usd);
    sem_wait(lock);
    ret = rpq[this->semval-1];
    rpq[this->semval-1] = NULL;
    this->semval -= 1;
    sem_post(empty);
    sem_post(lock);
    return ret;
}

template class rpqueue<Post*>;

