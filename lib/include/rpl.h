#ifndef RPL_H_
#define RPL_H_

#include <string>
#include <vector>

#include "link.h"
#include "post.h"
#include "acct.h"

class rpl_network;
class rpl_storage;
class rpl_con;

class rePoster{
public:
    class NewPostCB{
    public:
        virtual void Run(const Post&  p, const int rank)=0;
    };

    rePoster(){pnet=NULL;pstore=NULL;pcon=NULL;};
    ~rePoster();
    void init();
    void startRepost();
    void stopRepost();

    void sendPost(Post p);

    void setNewPostCB(NewPostCB *newPostCB){this->newPostCB = newPostCB;}; /* set cb */
    static void cb_wrap(void *reposter, Post *p, int rank);   

    std::vector<Account> getAccounts();
    void addAccount(Account newaccount);
    void rmAccount(Account account);

    std::vector<Link> getLinks();
    void addLink(Link newlink);
    void rmLink(Link link);

    void getInitialPosts(NewPostCB *newPostCB);

    void upboat(std::string uuid);
    void downboat(std::string uuid);
private:
    void cb(Post *p, int rank);   
    NewPostCB *newPostCB; /* callback made when we get new post */
    rpl_network *pnet;
    rpl_storage *pstore;
    rpl_con *pcon;

};

#endif
