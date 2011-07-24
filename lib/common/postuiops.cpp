#include "postuiops.h"
#include "rpdebug.h"

void PostUiOps::NewPost(Post *p, int rank)
{
    LOG_IF(WARNING, !newpostcb_) << "PostUiOp::NewPost == NULL";
    if(newpostcb_)newpostcb_->Run(*p, rank);
}

