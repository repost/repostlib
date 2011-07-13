#ifndef RP_SEMAPHORE_H_
#define RP_SEMAPHORE_H_

#include <pthread.h>
#include <semaphore.h>

class RpSemaphore
{
public:
    RpSemaphore(int value);
    ~RpSemaphore();
    bool TryWait();
    void Wait();
    void Post();
    int GetValue();

private:
    sem_t* semaphore_;
    sem_t* valsema_;
    int value_;
};

/*
** Rather than enclose contexts in Lock and Post we can
** just call this class and it does the magic for us
*/
class ContextLock
{
public:
    ContextLock(RpSemaphore* sema)
    :semaphore_(sema){semaphore_->Wait();}
    ~ContextLock(){semaphore_->Post();};
private:
    RpSemaphore* semaphore_;
};

#endif
