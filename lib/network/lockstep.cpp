#include "rpl.h"
#include "lockstep.h"
#include <string.h>

#ifdef OS_MACOSX
#define NAMED_SEMAPHORE
#include <time.h>
#endif

lockstep::lockstep()
{
#ifdef NAMED_SEMAPHORE
    char name[32];
    long now;
    now = (long) time(0);

    snprintf(name, 32, "/%s-%10d-%10ld", "lock", getpid(), now);
    this->boss = sem_open(name, O_CREAT, 0, 0);

    snprintf(name, 32, "/%s-%10d-%10ld", "usd", getpid(), now);
    this->spinner = sem_open(name, O_CREAT, 0, 0);

#else
    this->boss =  new sem_t;
    this->spinner =  new sem_t;
    sem_init(this->boss, 0, 0);
    sem_init(this->spinner, 0, 0);
#endif
    this->semval = 0;
}  

lockstep::~lockstep()
{
#ifdef NAMED_SEMAPHORE
    sem_close(boss);
    sem_close(spinner);
#else
    sem_destroy(boss);
    sem_destroy(spinner);
#endif
}

void lockstep::unlockBoss()
{
    sem_post(boss);
}

void lockstep::checkBoss()
{
    sem_wait(boss);
}

void lockstep::lockBoss()
{
    sem_post(spinner);
}

void lockstep::unlockSpinner()
{
    sem_post(spinner);
}

void lockstep::checkSpinner()
{
    sem_post(boss);
    sem_wait(spinner);
    sem_trywait(boss);
}

void lockstep::lockSpinner()
{
    sem_wait(spinner);
    sem_wait(boss);
}
