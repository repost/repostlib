#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <time.h>

#include "rpl.h"
#include "rpl_storage.h"
#include "rpdebug.h"

using namespace std;

const char rpl_storage::DATABASE_NAME[] = "repost.db";
const char rpl_storage::DROP_POST_TABLE[] = 
    "DROP TABLE posts;";
const char rpl_storage::CREATE_POST_TABLE[] = 
    "CREATE TABLE IF NOT EXISTS posts (id INTEGER PRIMARY KEY, author TEXT, \
    uuid TEXT, content TEXT, upvotes INTEGER, time INTEGER);";
const char rpl_storage::CREATE_VERSION_TABLE[] = 
    "CREATE TABLE IF NOT EXISTS version (version INTEGER);";
bool initialised = false;
int iRowsReturned = 0;

const int rpl_storage::CURRENT_VERSION_NUMBER = 3;
int db_version_number = -1;

rpl_storage *rpl_storage::INSTANCE = NULL; 

std::string rpl_storage::_db_location;

void print_error ( int err )
{
    switch (err)
    {
        case SQLITE_OK:
            LOG(INFO) << "SQLITE_OK";
            break;
        case SQLITE_ERROR:
            LOG(INFO) << "SQLITE_ERROR";
            break;
        case SQLITE_INTERNAL:
            LOG(INFO) << "SQLITE_INTERNAL";
            break;
        case SQLITE_PERM:
            LOG(INFO) << "SQLITE_PERM";
            break;
        case SQLITE_ABORT:
            LOG(INFO) << "SQLITE_ABORT";
            break;
        case SQLITE_BUSY:
            LOG(INFO) << "SQLITE_BUSY";
            break;
        case SQLITE_LOCKED:
            LOG(INFO) << "SQLITE_LOCKED";
            break;
        case SQLITE_NOMEM:
            LOG(INFO) << "SQLITE_NOMEM";
            break;
        case SQLITE_READONLY:
            LOG(INFO) << "SQLITE_READONLY";
            break;
        case SQLITE_INTERRUPT:
            LOG(INFO) << "SQLITE_INTERRUPT";
            break;
        case SQLITE_IOERR:
            LOG(INFO) << "SQLITE_IOERR";
            break;
        case SQLITE_CORRUPT:
            LOG(INFO) << "SQLITE_CORRUPT";
            break;
        case SQLITE_NOTFOUND:
            LOG(INFO) << "SQLITE_NOTFOUND";
            break;
        case SQLITE_FULL:
            LOG(INFO) << "SQLITE_FULL";
            break;
        case SQLITE_CANTOPEN:
            LOG(INFO) << "SQLITE_CANTOPEN";
            break;
        case SQLITE_PROTOCOL:
            LOG(INFO) << "SQLITE_PROTOCOL";
            break;
        case SQLITE_EMPTY:
            LOG(INFO) << "SQLITE_EMPTY";
            break;
        case SQLITE_SCHEMA:
            LOG(INFO) << "SQLITE_SCHEMA";
            break;
        case SQLITE_TOOBIG:
            LOG(INFO) << "SQLITE_TOOBIG";
            break;
        case SQLITE_CONSTRAINT:
            LOG(INFO) << "SQLITE_CONSTRAINT";
            break;
        case SQLITE_MISMATCH:
            LOG(INFO) << "SQLITE_MISMATCH";
            break;
        case SQLITE_MISUSE:
            LOG(INFO) << "SQLITE_MISUSE";
           break;
        case SQLITE_NOLFS:
            LOG(INFO) << "SQLITE_NOLFS";
            break;
        case SQLITE_AUTH:
            LOG(INFO) << "SQLITE_AUTH";
            break;
        case SQLITE_FORMAT:
            LOG(INFO) << "SQLITE_FORMAT";
            break;
        case SQLITE_RANGE:
            LOG(INFO) << "SQLITE_RANGE";
            break;
        case SQLITE_NOTADB:
            LOG(INFO) << "SQLITE_NOTADB";
            break;
        case SQLITE_ROW:
            LOG(INFO) << "SQLITE_ROW";
            break;
        case SQLITE_DONE:
            LOG(INFO) << "SQLITE_DONE";
            break;
    }
}

/**
 * @brief initialises the store 
 */
rpl_storage::rpl_storage()
{
    int rc = 0;

    LOG(DEBUG) << "> rpl_storage";
    rc = sqlite3_open( this->db_location(), &this->db );
    if ( rc )
    {
        LOG(WARNING) << "Couldn't open db " << this->db_location();
    }
    else
    {
        this->initialised = true;
        this->setup_tables();
    }

    sqlite3_close( this->db );
    LOG(DEBUG) << "< rpl_storage";
}

rpl_storage::~rpl_storage()
{
   
}

void rpl_storage::init(string dir)
{
    _db_location = dir.append("/");
    _db_location = dir.append(rpl_storage::DATABASE_NAME);
    INSTANCE = new rpl_storage();
}

rpl_storage *rpl_storage::get_instance(void)
{
    return INSTANCE;
}

const char* rpl_storage::db_location(void)
{
  return this->_db_location.c_str();
}

void rpl_storage::add_link (Link &link)
{

}

bool rpl_storage::add_post (Post *post)
{
    int rc = 0;
    bool ret = false;
    sqlite3_stmt *sql_stmt = NULL;
    time_t now = time ( NULL );
    const char * post_insert = "INSERT INTO posts ( uuid, content, time, "
        "upvotes) SELECT ?, ?, ?, 0 WHERE NOT EXISTS (SELECT * FROM posts "
        "WHERE posts.uuid = ?);";

    rc = sqlite3_open( this->db_location(), &this->db );
    if ( rc )
    {
        LOG(WARNING) << "Couldn't open db " << this->db_location();
        return ret;
    }

    rc = sqlite3_prepare_v2( this->db, post_insert, -1, &sql_stmt, NULL);
    if ( rc )
    {
        LOG(WARNING) << "Couldn't prepare insert statement err = " << rc 
                     << " db = " << this->db << " " << post_insert;
        return ret;
    }

    rc += sqlite3_bind_text(sql_stmt, 1, post->uuid().c_str(), post->uuid().length(), SQLITE_TRANSIENT);
    rc += sqlite3_bind_text(sql_stmt, 2, post->content().c_str(), post->content().length(), SQLITE_TRANSIENT);
    rc += sqlite3_bind_int(sql_stmt, 3, now); 
    rc += sqlite3_bind_text(sql_stmt, 4, post->uuid().c_str(), post->uuid().length(), SQLITE_TRANSIENT);
    if ( rc )
    {
        LOG(WARNING) << "Couldn't bind text and int accumlative err = " << rc;
        return ret;
    }

    // TODO Tidy up expressions here
    rc = sqlite3_step( sql_stmt );
    if ( rc != SQLITE_DONE )
    {
        LOG(WARNING) << "error insert: " << rc;
    }

    rc = sqlite3_finalize( sql_stmt );
    if ( rc != SQLITE_OK )
    {
        LOG(WARNING) << "error insert: " << rc;
    }

    if( sqlite3_changes(this->db) > 0 )
    {
        ret = true;
    }
    sqlite3_close( this->db );

    return ret;
}

void rpl_storage::get_link (Link *link)
{

}

int rpl_storage::postSelectArray(sqlite3_stmt* sql_stmt, Post** post)
{
	int rowsReturned = 0;
	while( sqlite3_step( sql_stmt ) == SQLITE_ROW)
	{
		int columns = sqlite3_column_count(sql_stmt);
		post[rowsReturned] = new Post();
		for ( int i = 0; i < columns; i ++ )
		{
			if (strstr(sqlite3_column_name(sql_stmt, i), "uuid" ) != NULL)
			{
				post[rowsReturned]->set_uuid(
						reinterpret_cast<const char*>(sqlite3_column_text(sql_stmt, i)));
			}
			else if ( strstr(sqlite3_column_name(sql_stmt, i), "content") 
					!= NULL )
			{
				post[rowsReturned]->set_content(
						reinterpret_cast<const char*>(sqlite3_column_text(sql_stmt, i)));
			}
		}
		rowsReturned++;
	}
	return rowsReturned;
}

int rpl_storage::get_post ( Post **post, string uuid )
{
    int rc = 0;
    int rowsReturned = 0;
    sqlite3_stmt *sql_stmt = NULL;
    const char* get_post = "SELECT * FROM posts WHERE posts.uuid = ?;";

    LOG(DEBUG) << "> get_post single";

    rc = sqlite3_open( this->db_location(), &this->db );
    if ( rc )
    {
        LOG(WARNING) << "Couldn't open db " << this->db_location();
        return rowsReturned;
    }


    rc = sqlite3_prepare_v2( this->db, get_post, -1, &sql_stmt, NULL);
    if ( rc )
    {
        LOG(WARNING) << "Couldn't prepare insert statement err = " << rc 
            << " db = " << this->db << " " << get_post;
        return rowsReturned;
    }

    rc = sqlite3_bind_text(sql_stmt, 1, uuid.c_str(), uuid.length(), SQLITE_TRANSIENT);
    if ( rc )
    {
        LOG(WARNING) << "Couldn't bind text and int accumlative err = " << rc;
        return rowsReturned;
    }

    rowsReturned = this->postSelectArray(sql_stmt, post);

    rc = sqlite3_finalize( sql_stmt );
    if ( rc != SQLITE_OK )
    {
        LOG(WARNING) << "error get_post: " <<  rc;
    }
    else
    {
        LOG(DEBUG) << "sqlite ok!";
    }

    sqlite3_close( this->db );

    LOG(DEBUG) << "< get_post";

    return rowsReturned;
}

int rpl_storage::get_post ( Post **post, int from, int count )
{
    int rc = 0 ;
    int rowsReturned = 0;
    sqlite3_stmt *sql_stmt = NULL;
    const char* get_post = "SELECT * FROM posts ORDER BY time DESC LIMIT ? "
        "OFFSET ?;" ;

    LOG(DEBUG) << "> get_post";

    rc = sqlite3_open( this->db_location(), &this->db );
    if ( rc )
    {
        LOG(WARNING) << "Couldn't open db " << this->db_location();
        return rowsReturned;
    }

    rc = sqlite3_prepare_v2( this->db, get_post, -1, &sql_stmt, NULL);
    if ( rc )
    {
        LOG(WARNING) << "Couldn't prepare insert statement err = " << rc 
            << " db = " << this->db << " " << get_post;
        return rowsReturned;
    }

    rc = sqlite3_bind_int(sql_stmt, 1, count);
    rc += sqlite3_bind_int(sql_stmt, 2, from);
    if ( rc )
    {
        LOG(WARNING) << "Couldn't bind text and int accumlative err = " << rc;
        return rowsReturned;
    }

    rowsReturned = this->postSelectArray(sql_stmt, post);

    rc = sqlite3_finalize( sql_stmt );
    if ( rc != SQLITE_OK )
    {
        LOG(WARNING) << "error insert: " << rc;
    }
    else
    {
        LOG(DEBUG) << "sqlite ok!";
    }

    sqlite3_close( this->db );

    LOG(DEBUG) << "< get_post";

    return rowsReturned;
}

int rpl_storage::check_version_number (void * id, int columns, char **column_text, char **column_name)
{
    int *version = (int *)id;
    for ( int i = 0; i < columns; i ++ )
    {
        if ( strstr ( column_name[i], "version" ) != NULL )
        {
            // get version number 
            *version = atoi(column_text[i]);
            LOG(INFO) <<  column_name[i] << " = " << column_text[i];
        }
    }
    return 0;
}

void rpl_storage::update_table ( )
{
    int rc;
    int version = -1;
    char *errmsg = NULL;

    LOG(DEBUG) << "> " << __FUNCTION__;

    rc = sqlite3_open( this->db_location(), &this->db );
    if ( rc )
    {
        LOG(WARNING) << "Couldn't open db " << this->db_location();
        return;
    }

    /* drop posts table */
    /* TODO: migrate users to new table */
    rc = sqlite3_exec ( this->db, 
            "PRAGMA user_version;", 
            &rpl_storage::check_version_number,
            &version,
            &errmsg);

    if ( errmsg != NULL )
    {
        LOG(WARNING) << "error read version table: " <<  errmsg;
        sqlite3_free ( errmsg );
    }

    /* if version is different  */
    // TODO: setup migration!
    if ( version != CURRENT_VERSION_NUMBER )
    {
        stringstream sql_stmt;

        // create new posts table
        rc = sqlite3_exec ( this->db, 
                this->DROP_POST_TABLE, 
                NULL,
                NULL,
                &errmsg);

        if ( errmsg != NULL )
        {
            LOG(WARNING) << "error drop post table: " << errmsg;
            sqlite3_free ( errmsg );
        }

        // create new posts table
        rc = sqlite3_exec ( this->db, 
                this->CREATE_POST_TABLE, 
                NULL,
                NULL,
                &errmsg);

        if ( errmsg != NULL )
        {
            LOG(WARNING) << "error create post table: " << errmsg;
            sqlite3_free ( errmsg );
        }

        sql_stmt << "PRAGMA user_version = " << CURRENT_VERSION_NUMBER;
        // update db with new version number 
        rc = sqlite3_exec ( this->db, 
                sql_stmt.str().c_str(), 
                NULL,
                NULL,
                &errmsg);

        if ( errmsg != NULL )
        {
            LOG(WARNING) << "error update version table: " <<  errmsg;
            sqlite3_free ( errmsg );
        }
    }

    LOG(DEBUG) << "< " << __FUNCTION__;
    sqlite3_close( this->db );
}

bool rpl_storage::setup_tables ()
{
    int rc;
    char *errmsg = NULL;
    bool ret = true;

    LOG(DEBUG) << ">" << __FUNCTION__;

    rc = sqlite3_exec ( this->db, 
            CREATE_POST_TABLE, 
            NULL,
            NULL,
            &errmsg);

    if ( errmsg != NULL )
    {
        LOG(WARNING) << "error create post table: " << errmsg;
        sqlite3_free ( errmsg );
        ret = false;
    }

    // check version numbers and stuff
    this->update_table( );

    LOG(DEBUG) << "< " <<  __FUNCTION__;

    return true;
}

void rpl_storage::delete_post ( string uuid )
{
    int rc;
    char *errmsg;

    LOG(DEBUG) << __FUNCTION__ << " do some shit here with uuid " << uuid;
    stringstream sql_stmt;

    rc = sqlite3_open( this->db_location(), &this->db );
    if ( rc )
    {
        LOG(WARNING) << "Couldn't open db " << this->db_location();
        return;
    }
    
    sql_stmt << "DELETE FROM posts WHERE uuid = \"" << uuid << "\"";

    rc = sqlite3_exec ( this->db, 
                        sql_stmt.str().c_str(), 
                        NULL, 
                        NULL, 
                        &errmsg );

    if ( rc != SQLITE_OK )
    {
        LOG(WARNING) << "sqlite error! error number " << rc;
    }
    else
    {
        LOG(DEBUG) << "sqlite ok!";
    }
    sqlite3_close( this->db );
}

void rpl_storage::update_metric ( string uuid )
{
    int rc;
    char *errmsg;
    stringstream sql_stmt;

    LOG(DEBUG) << __FUNCTION__ << " do some shit here with uuid " << uuid;
    rc = sqlite3_open( this->db_location(), &this->db );
    if ( rc )
    {
        LOG(WARNING) << "Couldn't open db " << this->db_location();
        return;
    }

    sql_stmt << "UPDATE posts SET upvotes = upvotes + 1 WHERE uuid = \"" << uuid << "\"";

    rc = sqlite3_exec ( this->db, 
                        sql_stmt.str().c_str(), 
                        NULL, 
                        NULL, 
                        &errmsg );

    if ( rc != SQLITE_OK )
    {
        LOG(WARNING) << "sqlite error! error number " << rc;
    }
    else
    {
        LOG(DEBUG) << "sqlite ok!";
    }
    sqlite3_close( this->db );
}
