#ifndef RPL_STORAGE_H_
#define RPL_STORAGE_H_

#include <string>
#include <sqlite3.h>
#include "rpl.h"
#include "link.h"

class rpl_storage
{
  public:
    rpl_storage();
    static rpl_storage *get_instance ();
    void add_link(Link &link);
    void add_post (Post *post);
    void get_link(Link *link);
    int get_post ( Post **post, int len, int count );
    void delete_post ( std::string uuid );
    void update_metric ( std::string uuid );
    static rpl_storage *INSTANCE;
  private:
    ~rpl_storage();
    sqlite3 *db;
    static const char DATABASE_NAME[];
    static const char DROP_POST_TABLE[];
    static const char CREATE_POST_TABLE[];
    static const char CREATE_VERSION_TABLE[];
    bool initialised;
    bool setup_tables();
    void update_table( );
    static const int CURRENT_VERSION_NUMBER;
    static int check_version_number (void * id, int columns, char **column_text, char **column_name);
};

#endif
