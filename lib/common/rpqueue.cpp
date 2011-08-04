#include "rpl.h"
#include "rpqueue.h"
#include "rpdebug.h"

template <class obj>
rpqueue<obj>::rpqueue()
{
    empty_ =  new RpSemaphore(QUEUE_SIZE);
    lock_ =  new RpSemaphore(1);
    usd_ =  new RpSemaphore(0);
    pos_ = 0;
}  

template <class obj>
rpqueue<obj>::~rpqueue()
{
    delete empty_;
    delete lock_;
    delete usd_;
}

template <class obj>
void rpqueue<obj>::add(obj newobj)
{
    empty_->Wait();
    lock_->Wait();
    LOG(DEBUG) << "Adding to queue at " << pos_;
    rpq[pos_] = newobj;
    pos_ += 1;
    usd_->Post();
    lock_->Post();
}

template <class obj>
obj rpqueue<obj>::get()
{
    obj ret;
    usd_->Wait();
    lock_->Wait();
    pos_ -= 1;
    LOG(DEBUG) << "Getting from queue at " << pos_;
    ret = rpq[pos_];
    rpq[pos_] = NULL;
    empty_->Post();
    lock_->Post();
    return ret;
}

template class rpqueue<Post*>;
