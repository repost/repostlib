
#include "rpsemaphore.h"
#include "rpdebug.h"
#include "string.h"
#ifdef OS_MACOSX
#define NAMED_SEMAPHORE
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include "stdio.h"
#endif


RpSemaphore::RpSemaphore(int value)
    : value_(value)
{
#ifdef NAMED_SEMAPHORE
    char name[32];
    long now;
    now = (long) time(0);
    srand(clock());
    snprintf(name, 32, "/%s-%04d-%10ld", "rpsema", rand(), now);
    semaphore_ = sem_open(name, O_CREAT, 0, value);
    if( semaphore_ == SEM_FAILED)
    {
      printf("SEMAPHORE FAILED TO BE CREATED err %s\n", strerror(errno));
    }
    snprintf(name, 32, "/%s-%04d-%10ld", "value", rand(), now);
    valsema_ = sem_open(name, O_CREAT, 0, 1);
    if( semaphore_ == SEM_FAILED)
    {
      printf("SEMAPHORE VALUE FAILED TO BE CREATED err %d\n",errno);
    }
#else
    semaphore_ = new sem_t;
    sem_init(semaphore_, 0, value_);
#endif
}

RpSemaphore::~RpSemaphore()
{
#ifdef NAMED_SEMAPHORE
    sem_close(semaphore_);
    sem_close(valsema_);
#else
    sem_destroy(semaphore_);
#endif
}

bool RpSemaphore::TryWait()
{
#ifdef NAMED_SEMAPHORE
    if(sem_trywait(semaphore_))
    {
        sem_wait(valsema_);
        value_ -= 1;
        sem_post(valsema_);
    }
#else
    return sem_trywait(semaphore_);
#endif
}

void RpSemaphore::Wait()
{
    sem_wait(semaphore_);
#ifdef NAMED_SEMAPHORE
    sem_wait(valsema_);
    value_ -= 1;
    sem_post(valsema_);
#endif
}

void RpSemaphore::Post()
{
    sem_post(semaphore_);
#ifdef NAMED_SEMAPHORE
    sem_wait(valsema_);
    value_ += 1;
    sem_post(valsema_);
#endif
}

int RpSemaphore::GetValue()
{
    int ret; 
#ifdef NAMED_SEMAPHORE
    sem_wait(valsema_);
    ret = value_;
    sem_post(valsema_);
#else
    sem_getvalue(semaphore_, &ret);
    ret = ret < 0 ? 0 : ret;
#endif
    return ret;
}

