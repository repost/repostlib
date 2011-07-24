#ifndef POSTUIOPS_H_
#define POSTUIOPS_H_

class PostUiOps{
public:
    PostUiOps(): 
        newpost = NULL {};
/* Callbacks */
    class NewPostCB{
    public:
        virtual void Run(const Post&  p, const int rank)=0;
    };
    void NewPost(Post *p, int rank);   

private:
    NewPostCB *newpost_;
};

#endif
