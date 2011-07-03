#ifndef LOCKSTEP_H_
#define LOCKSTEP_H_

#include <pthread.h>
#include <semaphore.h>

extern "C" {
#include "glib.h"
}

class LockStep
{
public:
    LockStep();
    ~LockStep();
    GSource* GlibEventSource();
    void CheckSpinner();
    void UnlockSpinner();
    void LockSpinner();

private:
    sem_t* boss;
    sem_t* spinner;
    sem_t* lock;
};


#endif
