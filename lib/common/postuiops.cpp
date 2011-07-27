#include "postuiops.h"
#include "rpdebug.h"

void PostUiOps::NewPost(Post *p, int rank)
{
    LOG_IF(WARNING, !newpostcb_) << "PostUiOp::NewPost == NULL. \
        Callback thrown away.";
    if(newpostcb_)newpostcb_->Run(*p, rank);
}

void PostUiOps::PostMetric(std::string uuid, int metric)
{
    LOG_IF(WARNING, !newpostcb_) << "PostUiOp::PostMetric == NULL \
        Callback thrown away.";
    if(postmetriccb_)postmetriccb_->Run(uuid, metric);
}
