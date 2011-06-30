#include "rpl.h"
#include "lockstep.h"
#include <string.h>
#include "stdio.h"

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
    this->semval = 0;
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

void LockStep::UnlockBoss()
{
    sem_post(boss);
}

void LockStep::CheckBoss()
{
    sem_wait(boss);
}

void LockStep::LockBoss()
{
    sem_post(spinner);
}

void LockStep::UnlockSpinner()
{
    printf("unlocking spinner\n");
    sem_post(spinner);
}

void LockStep::CheckSpinner()
{
    printf("checking spinner\n");
    sem_post(boss);
    printf("waiting on spinner spinner\n");
    sem_wait(spinner);
    sem_post(spinner);
    sem_trywait(boss);
    printf("finished checking spinner\n");
}

void LockStep::LockSpinner()
{	
    printf("locking spinner\n");
    sem_trywait(spinner);
    sem_wait(boss);
    printf("finsihed locking spinner\n");
}
