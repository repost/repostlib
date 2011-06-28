#include "rpl.h"
#include "lockstep.h"
#include <string.h>
#include "stdio.h"

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
    sem_init(this->spinner, 0, 1);
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
		printf("unlocking spinner\n");
    sem_post(spinner);
}

void lockstep::checkSpinner()
{
		printf("checking spinner\n");
    sem_post(boss);
		printf("waiting on spinner spinner\n");
    sem_wait(spinner);
    sem_post(spinner);
    sem_trywait(boss);
		printf("finished checking spinner\n");
}

void lockstep::lockSpinner()
{	
		printf("locking spinner\n");
    sem_trywait(spinner);
    sem_wait(boss);
		printf("finsihed locking spinner\n");
}
