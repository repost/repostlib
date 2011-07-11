
#include "rpsemaphore.h"
#ifdef OS_MACOSX
#define NAMED_SEMAPHORE
#include <time.h>
#endif


RpSemaphore::RpSemaphore(int value)
    : value_(value)
{
#ifdef NAMED_SEMAPHORE
    char name[32];
    long now;
    now = (long) time(0);
    snprintf(name, 32, "/%s-%10d-%10ld", "rpsema", getpid(), now);
    semaphore_ = sem_open(name, O_CREAT, 0, 0);
    snprintf(name, 32, "/%s-%10d-%10ld", "valuelock", getpid(), now);
    valsema_ = sem_open(name, O_CREAT, 0, 0);
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

void RpSemaphore::TryWait()
{
    if(sem_trywait(semaphore_))
    {
#if NAMED_SEMAPHORE
        sem_wait(valsema_);
        value_ -= 1;
        sem_post(valsema_);
#endif
    }
}

void RpSemaphore::Wait()
{
    sem_wait(semaphore_);
#if NAMED_SEMAPHORE
    sem_wait(valsema_);
    value_ -= 1;
    sem_post(valsema_);
#endif
}

void RpSemaphore::Post()
{
    sem_post(semaphore_);
#if NAMED_SEMAPHORE
    sem_wait(valsem_);
    value_ += 1;
    sem_post(valsem_);
#endif
}

int RpSemaphore::GetValue()
{
    int ret; 
#if NAMED_SEMAPHORE
    sem_wait(valsem_);
    ret = value_;
    sem_post(valsem_);
#else
    ret = sem_getvalue(semaphore_, &ret);
#endif
    return ret;
}

