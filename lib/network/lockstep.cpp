#include "rpl.h"
#include "lockstep.h"
#include <string.h>
#include "stdio.h"
#include "rpdebug.h"

#ifdef OS_MACOSX
#define NAMED_SEMAPHORE
#include <time.h>
#endif

LockStep::LockStep()
{
#ifdef NAMED_SEMAPHORE
    char name[32];
    long now;
    now = (long) time(0);
    snprintf(name, 32, "/%s-%10d-%10ld", "boss", getpid(), now);
    this->boss = sem_open(name, O_CREAT, 0, 0);
    snprintf(name, 32, "/%s-%10d-%10ld", "spinner", getpid(), now);
    this->spinner = sem_open(name, O_CREAT, 0, 1);
#else
    this->boss = new sem_t;
    this->spinner = new sem_t;
    sem_init(this->boss, 0, 0);
    sem_init(this->spinner, 0, 1);
#endif
}  

LockStep::~LockStep()
{
#ifdef NAMED_SEMAPHORE
    sem_close(boss);
    sem_close(spinner);
#else
    sem_destroy(boss);
    sem_destroy(spinner);
#endif
}

void LockStep::UnlockSpinner()
{
    LOG(DEBUG) << "unlocking spinner";
    sem_post(spinner);
}

void LockStep::CheckSpinner()
{
    LOG(DEBUG) << "checking spinner";
    sem_post(boss);
    LOG(DEBUG) << "waiting on spinner";
    sem_wait(spinner);
    sem_post(spinner);
    sem_trywait(boss);
    LOG(DEBUG) << "finished checking spinner";
}

void LockStep::LockSpinner()
{	
    LOG(DEBUG) << "trywait locking spinner";
    sem_trywait(spinner);
    LOG(DEBUG) << "finsihed locking spinner";
}

void LockStep::CheckBoss()
{	
    LOG(DEBUG) << "checking boss";
    sem_wait(boss);
    LOG(DEBUG) << "finsihed checking boss";
}
