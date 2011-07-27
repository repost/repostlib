#ifndef POSTUIOPS_H_
#define POSTUIOPS_H_

#include <stddef.h>
#include <string>

class Post;

class PostUiOps{
public:
    PostUiOps(): newpostcb_(NULL),
        postmetriccb_(NULL){};

/* Callbacks */
    class NewPostCB{
    public:
        virtual void Run(const Post &p, const int rank)=0;
    };
    void NewPost(Post *p, int rank);   
    void set_newpostcb(NewPostCB* npcb){newpostcb_ = npcb;};

    class PostMetricCB{
    public:
        virtual void Run(const std::string &uuid, const int metric)=0;
    };
    void PostMetric(std::string uuid, int metric);
    void set_postmetriccb(PostMetricCB* pmcb){postmetriccb_ = pmcb;};

private:
    NewPostCB *newpostcb_;
    PostMetricCB *postmetriccb_;
};

#endif
