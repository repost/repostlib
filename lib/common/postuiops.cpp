#include "postuiops.h"

void PostUiOps::NewPost(Post *p, int rank)
{
    this->newpost_->Run(*p, rank);
}

