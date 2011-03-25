#ifndef RPL_STORAGE_H_
#define RPL_STORAGE_H_

#include <sqlite3.h>
#include "rpl.h"

class rpl_storage
{
  public:
    rpl_storage *get_instance ();
    void add_link(rp_link &link);
    void add_friends(rp_friend &my_friend);
    void add_network(rp_network &network);
    void add_post (Post &post);
    void get_link(rp_link *link);
    void get_friends(rp_friend *my_friend);
    void get_network(rp_network *network);
    void get_post (Post *post, int len, int count, void *callback);
  private:
    rpl_storage();
    ~rpl_storage();
    sqlite3 *db;
    static const char DATABASE_NAME[];
    static const char CREATE_POST_TABLE[];
    static rpl_storage *INSTANCE;
    bool initialised;
    bool setup_tables();
};

#endif
