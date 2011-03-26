#ifndef RPL_H_
#define RPL_H_

#include <string>
#include <vector>

#include "link.h"
#include "post.h"

class rpl_network;
class rpl_storage;
class rpl_con;

class rePoster{
public:
    class NewPostCB{
    public:
        virtual void Run(const Post&  p)=0;
    };

    rePoster() {};
    void init();
    void startRepost();
    void stopRepost();

    Post newPost();
    void sendPost(Post p);

    void setNewPostCB(NewPostCB *newPostCB){this->newPostCB = newPostCB;}; /* set cb */
    static void cb_wrap(void *reposter, Post *p);   

    void addAccount(std::string user, std::string pass, std::string network);
    std::vector<Link> getLinks();

private:
    void cb(Post *p);   
    NewPostCB *newPostCB; /* callback made when we get new post */
    rpl_network *pnet;
    rpl_storage *pstore;
    rpl_con *pcon;

};

#endif
