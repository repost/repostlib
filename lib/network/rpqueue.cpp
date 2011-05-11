
#include "rpl.h"
#include "rpqueue.h"
#include <string.h>

#ifdef __OSX__
#define NAMED_SEMAPHORE
#endif

rpqueue::rpqueue()
{
#ifdef NAMED_SEMAPHORE
    char name[32];
    snprintf(name, 32, "/%s-%d", "lock", getpid());
    this->lock = sem_open(name, O_CREAT, 0, 1);

    snprintf(name, 32, "/%s-%d", "usd", getpid());
    this->usd = sem_open(name, O_CREAT, 0, 0);

    snprintf(name, 32, "/%s-%d", "empty", getpid());
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
    postq[QUEUE_SIZE - this->semval - 1] = post;
    this->semval += 1;
    sem_post(usd);
    sem_post(lock);
}

Post * rpqueue::get()
{
    Post *ret = NULL;
    sem_wait(usd);
    sem_wait(lock);
    ret = postq[this->semval];
    postq[this->semval] = NULL;
    semval -= 1;
    sem_post(empty);
    sem_post(lock);
    return ret;
}
