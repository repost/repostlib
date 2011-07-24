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
    ~rpl_storage();
    static rpl_storage *get_instance ();
    static void init (std::string dir);
    static void uninit ();
    void add_link(Link &link);
    bool add_post (Post *post); /* return true if new */
    void get_link(Link *link);
    int get_post ( Post **post, int len, int count );
    int get_post ( Post **post, std::string uuid );
    void delete_post ( std::string uuid );
    void update_metric ( std::string uuid );
    static rpl_storage *INSTANCE;
    bool update_account ( Account *olddetails, Account *newdetails );
    bool delete_account ( Account *account);
    bool add_account ( Account *account);
  private:
    sqlite3 *db;
    static std::string _db_location;
    static const char DATABASE_NAME[];
    static const char DROP_POST_TABLE[];
    static const char DROP_ACCOUNT_TABLE[];
    static const char CREATE_POST_TABLE[];
    static const char CREATE_ACCOUNT_TABLE[];
    static const char CREATE_VERSION_TABLE[];
    const char *db_location(void);
    bool initialised;
    bool setup_tables();
    void update_table( );
    bool db_check();
    static const int CURRENT_VERSION_NUMBER;
    static int check_version_number (void * id, int columns, 
        char **column_text, char **column_name);
    int postSelectArray(sqlite3_stmt* sql_stmt, Post** post);
};

#endif
