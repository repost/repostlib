
#include "rpsemaphore.h"
#include "string.h"
#ifdef OS_MACOSX
#define NAMED_SEMAPHORE
#include <time.h>
#include <unistd.h>
#include "stdio.h"
#endif


RpSemaphore::RpSemaphore(int value)
    : value_(value)
{
#ifdef NAMED_SEMAPHORE
    char name[32];
    long now;
    now = (long) time(0);
    snprintf(name, 32, "/%s-%10d-%10ld", "rpsema", getpid(), now);
    semaphore_ = sem_open(name, O_CREAT, 0, value);
    snprintf(name, 32, "/%s-%10d-%10ld", "valuelock", getpid(), now);
    valsema_ = sem_open(name, O_CREAT, 0, 1);
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
    ret = sem_getvalue(semaphore_, &ret);
#endif
    return ret;
}

