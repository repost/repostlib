#include "postuiops.h"

void PostUiOps::NewPost(Post *p, int rank)
{
    this->newpostcb_->Run(*p, rank);
}

