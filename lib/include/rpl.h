#ifndef RPL_H_
#define RPL_H_

#include <string>

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

private:
    void cb(Post *p);   
    NewPostCB *newPostCB; /* callback made when we get new post */
    rpl_network *pnet;
    rpl_storage *pstore;
    rpl_con *pcon;

};

class rp_link
{
public:
    rp_link(std::string linkname):linkname(linkname){};
    std::string get_name(){ return linkname;};
private:
    std::string linkname;
};
class rp_network
{
public:
    std::string get_title();
private:
    std::string title;
};

class rp_friend
{
public:
    std::string get_name();

private:
    std::string name;
    rp_network network;

};
#endif
