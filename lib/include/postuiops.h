#ifndef POSTUIOPS_H_
#define POSTUIOPS_H_

#include <stddef.h>
class Post;

class PostUiOps{
public:
    PostUiOps(): newpostcb_(NULL){};

/* Callbacks */
    class NewPostCB{
    public:
        virtual void Run(const Post&  p, const int rank)=0;
    };
    void NewPost(Post *p, int rank);   
    void setNewPostCB(NewPostCB* npcb){newpostcb_ = npcb;};

private:
    NewPostCB *newpostcb_;
};

#endif
