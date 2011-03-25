
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
    lock = sem_open(name,O_CREAT,0,1);

    snprintf(name, 32, "/%s-%d", "usd", getpid());
    usd = sem_open(name,O_CREAT,0,0);

    snprintf(name, 32, "/%s-%d", "empty", getpid());
    empty = sem_open(name,O_CREAT,0,QUEUE_SIZE);
#else
    empty =  new sem_t;
    lock =  new sem_t;
    usd =  new sem_t;
    sem_init(empty,0,QUEUE_SIZE);
    sem_init(lock,0,1);
    sem_init(usd,0,0);
#endif
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
    int pos = 0;
    sem_wait(empty);
    sem_wait(lock);
    sem_getvalue(empty, &pos);
    postq[QUEUE_SIZE - pos-1] = post;
    sem_post(usd);
    sem_post(lock);
}

Post * rpqueue::get()
{
    int pos = 0;
    Post *ret = NULL;
    sem_wait(usd);
    sem_wait(lock);
    sem_getvalue(usd, &pos);
    ret = postq[pos];
    postq[pos] = NULL;
    sem_post(empty);
    sem_post(lock);
    return ret;
}
