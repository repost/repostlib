
#include "rpl.h"
#include "rpqueue.h"
#include <string.h>

#ifdef OS_MACOSX
#define NAMED_SEMAPHORE
#include <time.h>
#endif

rpqueue::rpqueue()
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

rpqueue::~rpqueue()
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

void rpqueue::add(Post *post)
{
    sem_wait(empty);
    sem_wait(lock);
    postq[this->semval] = post;
    this->semval += 1;
    sem_post(usd);
    sem_post(lock);
}

Post * rpqueue::get()
{
    Post *ret = NULL;
    sem_wait(usd);
    sem_wait(lock);
    ret = postq[this->semval-1];
    postq[this->semval-1] = NULL;
    this->semval -= 1;
    sem_post(empty);
    sem_post(lock);
    return ret;
}
