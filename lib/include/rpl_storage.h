#ifndef RPL_STORAGE_H_
#define RPL_STORAGE_H_

#include <sqlite3.h>
#include "rpl.h"
#include "link.h"

class rpl_storage
{
  public:
    static rpl_storage *get_instance ();
    void add_link(Link &link);
    void add_post (Post *post);
    void get_link(Link *link);
    int get_post ( Post **post, int len, int count );
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
