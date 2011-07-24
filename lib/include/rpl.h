#ifndef RPL_H_
#define RPL_H_

#include <string>
#include <vector>

#include "link.h"
#include "post.h"
#include "acct.h"

#ifdef WIN32
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

class rpl_network;
class rpl_storage;
class rpl_con;
class PostUiOps;

class rePoster{
public:
    rePoster(): pnet=NULL, pstore=NULL, pcon=NULL;{};
    ~rePoster();
    void init();
    void startRepost();
    void stopRepost();

    void sendPost(Post p);

    std::vector<Account> getAccounts();
    void addAccount(Account newaccount);
    void rmAccount(Account account);

    std::vector<Link> getLinks();
    void addLink(Link newlink);
    void rmLink(Link link);

    void getInitialPosts(NewPostCB *newPostCB);
    std::string GetUserDir(void);

    void upboat(std::string uuid);
    void downboat(std::string uuid);
private:
    rpl_network *pnet;
    rpl_storage *pstore;
    rpl_con *pcon;
};

#endif
