#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <time.h>

#include "rpl.h"
#include "rpl_storage.h"

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
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_ERROR:
            cout << "SQLITE_ERROR" << endl;
            break;
        case SQLITE_INTERNAL:
            cout << "SQLITE_INTERNAL" << endl;
            break;
        case SQLITE_PERM:
            cout << "SQLITE_PERM" << endl;
            break;
        case SQLITE_ABORT:
            cout << "SQLITE_ABORT" << endl;
            break;
        case SQLITE_BUSY:
            cout << "SQLITE_BUSY" << endl;
            break;
        case SQLITE_LOCKED:
            cout << "SQLITE_LOCKED" << endl;
            break;
        case SQLITE_NOMEM:
            cout << "SQLITE_NOMEM" << endl;
            break;
        case SQLITE_READONLY:
            cout << "SQLITE_READONLY" << endl;
            break;
        case SQLITE_INTERRUPT:
            cout << "SQLITE_INTERRUPT" << endl;
            break;
        case SQLITE_IOERR:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_CORRUPT:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_NOTFOUND:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_FULL:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_CANTOPEN:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_PROTOCOL:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_EMPTY:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_SCHEMA:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_TOOBIG:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_CONSTRAINT:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_MISMATCH:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_MISUSE:
            cout << "SQLITE_OK" << endl;
           break;
        case SQLITE_NOLFS:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_AUTH:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_FORMAT:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_RANGE:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_NOTADB:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_ROW:
            cout << "SQLITE_OK" << endl;
            break;
        case SQLITE_DONE:
            cout << "SQLITE_OK" << endl;
            break;
    }
}

/**
 * @brief initialises the store 
 */
rpl_storage::rpl_storage()
{
    int rc;

    printf( "> rpl_storage\n" );
    rc = sqlite3_open( this->db_location(), &this->db );
    if ( rc )
    {
        fprintf( stderr, "Couldn't open db %s\n", 
            this->db_location() );
    }
    else
    {
        this->initialised = true;
        this->setup_tables();
    }

    sqlite3_close( this->db );
    printf( "< rpl_storage\n" );
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
    int rV = 0;
    bool ret = false;
    char *errmsg = NULL;
		sqlite3_stmt *sql_stmt = NULL;
    time_t now = time ( NULL );
		const char * post_insert = "INSERT INTO posts ( uuid, content, time ) SELECT "
              "?, ?, ? WHERE NOT EXISTS (SELECT * FROM posts WHERE posts.uuid = ?);";

    rc = sqlite3_open( this->db_location(), &this->db );
    if ( rc )
    {
        fprintf( stderr, "Couldn't open db %s\n", 
            this->db_location() );
        return ret;
    }
		
		rc = sqlite3_prepare_v2( this->db, post_insert, -1, &sql_stmt, NULL);
		if ( rc )
    {
        fprintf( stderr, "Couldn't prepare insert statement err = %d, db = 0x%x'%s'\n", 
																																rc, this->db, post_insert);
        return ret;
    }
		
		rc += sqlite3_bind_text(sql_stmt, 1, post->content().c_str(), post->uuid().length(), SQLITE_TRANSIENT);
		rc += sqlite3_bind_text(sql_stmt, 2, post->uuid().c_str(), post->content().length(), SQLITE_TRANSIENT);
		rc += sqlite3_bind_int(sql_stmt, 3, now); 
		rc += sqlite3_bind_text(sql_stmt, 4, post->uuid.c_str(), post->uuid().length(), SQLITE_TRANSIENT);
		if ( rc )
    {
        fprintf( stderr, "Couldn't bind text and int accumlative err = %d", rc);
        return ret;
    }
	
    rV = sqlite3_step( sql_stmt );
    rV += sqlite3_finalize( sql_stmt );
    if ( rV != SQLITE_OK )
    {
        printf( "error insert: %d\n", rV );
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



int print_post (void * id, int columns, char **column_text, char **column_name)
{
    static Post **post = NULL;
    static int n = 0;
    if ( post != (Post **)id )
    {
        post = (Post **)id;
        n = 0;
    }
    post[n] = new Post();

    for ( int i = 0; i < columns; i ++ )
    {
        if ( strstr ( column_name[i], "uuid" ) != NULL )
        {
            post[n]->set_uuid ( column_text[i] );
        }
        else if ( strstr ( column_name[i], "content" ) != NULL )
        {
            post[n]->set_content ( column_text[i] );
        }
    }
    n++;
    iRowsReturned++;
    return 0;
}

int rpl_storage::get_post ( Post **post, string uuid )
{
    int rc, rV;
    char *errmsg;
    stringstream sql_stmt;

    cout << "> get_post single" << endl;

    rc = sqlite3_open( this->db_location(), &this->db );
    if ( rc )
    {
        fprintf( stderr, "Couldn't open db %s\n", 
            this->db_location() );
        return 0;
    }

    iRowsReturned = 0;

    sql_stmt << "SELECT * FROM posts WHERE posts.uuid = \"" << uuid << "\" ;"; 

    rV = sqlite3_exec ( this->db, 
                        sql_stmt.str().c_str(), 
                        print_post, 
                        (void *)post, 
                        &errmsg );
    if ( rV != SQLITE_OK )
    {
        cout << "sqlite error! error number " << rV << endl;
    }
    else
    {
        cout << "sqlite ok!" << endl;
    }

    if ( errmsg != NULL )
    {
        printf( "error get_post: %s\n", errmsg );
        sqlite3_free ( errmsg );
    }

    sqlite3_close( this->db );

    cout << "< get_post" << endl;

    return iRowsReturned;
}

int rpl_storage::get_post ( Post **post, int from, int count )
{
    int rc, rV;
    char *errmsg;
    stringstream sql_stmt;

    cout << "> get_post" << endl;

    if ( post == NULL || count == 0 )
    {
        return iRowsReturned;
    }

    iRowsReturned = 0;

    rc = sqlite3_open( rpl_storage::DATABASE_NAME, &this->db );
    if ( rc )
    {
        fprintf( stderr, "Couldn't open db %s\n", 
            this->db_location() );
        return 0;
    }

    sql_stmt << "SELECT * FROM posts ORDER BY time ASC LIMIT " << count 
        << " OFFSET " << from << ";"; 

    rV = sqlite3_exec ( this->db, 
                        sql_stmt.str().c_str(), 
                        print_post, 
                        (void *)post, 
                        &errmsg );
    if ( rV != SQLITE_OK )
    {
        cout << "sqlite error! error number " << rV << endl;
    }
    else
    {
        cout << "sqlite ok!" << endl;
    }

    if ( errmsg != NULL )
    {
        printf( "error get_post: %s\n", errmsg );
        sqlite3_free ( errmsg );
    }

    sqlite3_close( this->db );

    cout << "< get_post" << endl;

    return iRowsReturned;
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
            printf ( "%s = %s\n", column_name[i], column_text[i] );
        }
    }
    return 0;
}

void rpl_storage::update_table ( )
{
    int rV;
    int version = -1;
    char *errmsg = NULL;

    cout << "> " << __FUNCTION__ << endl;

    rV = sqlite3_open( this->db_location(), &this->db );
    if ( rV )
    {
        fprintf( stderr, "Couldn't open db %s\n", 
            this->db_location() );
        return;
    }

    /* drop posts table */
    /* TODO: migrate users to new table */
    rV = sqlite3_exec ( this->db, 
            "PRAGMA user_version;", 
            &rpl_storage::check_version_number,
            &version,
            &errmsg);

    if ( errmsg != NULL )
    {
        printf( "error read version table: %s\n", errmsg );
        sqlite3_free ( errmsg );
    }

    /* if version is different  */
    // TODO: setup migration!
    if ( version != CURRENT_VERSION_NUMBER )
    {
        stringstream sql_stmt;

        // create new posts table
        rV = sqlite3_exec ( this->db, 
                this->DROP_POST_TABLE, 
                NULL,
                NULL,
                &errmsg);

        if ( errmsg != NULL )
        {
            printf( "error drop post table: %s\n", errmsg );
            sqlite3_free ( errmsg );
        }

        // create new posts table
        rV = sqlite3_exec ( this->db, 
                this->CREATE_POST_TABLE, 
                NULL,
                NULL,
                &errmsg);

        if ( errmsg != NULL )
        {
            printf( "error create post table: %s\n", errmsg );
            sqlite3_free ( errmsg );
        }

        sql_stmt << "PRAGMA user_version = " << CURRENT_VERSION_NUMBER;
        // update db with new version number 
        rV = sqlite3_exec ( this->db, 
                sql_stmt.str().c_str(), 
                NULL,
                NULL,
                &errmsg);

        if ( errmsg != NULL )
        {
            printf( "error update version table: %s\n", errmsg );
            sqlite3_free ( errmsg );
        }
    }

    cout << "< " << __FUNCTION__ << endl;
    sqlite3_close( this->db );
}

bool rpl_storage::setup_tables ()
{
    int rV;
    char *errmsg = NULL;
    bool ret = true;

    printf( "> %s\n", __FUNCTION__);

    rV = sqlite3_exec ( this->db, 
            CREATE_POST_TABLE, 
            NULL,
            NULL,
            &errmsg);

    if ( errmsg != NULL )
    {
        printf( "error create post table: %s\n", errmsg );
        sqlite3_free ( errmsg );
        ret = false;
    }

    // check version numbers and stuff
    this->update_table( );

    printf( "< %s\n", __FUNCTION__ );

    return true;
}

void rpl_storage::delete_post ( string uuid )
{
    int rV;
    char *errmsg;

    cout << __FUNCTION__ << " do some shit here with uuid " << uuid  << endl;
    stringstream sql_stmt;

    rV = sqlite3_open( this->db_location(), &this->db );
    if ( rV )
    {
        fprintf( stderr, "Couldn't open db %s\n", 
            this->db_location() );
        return;
    }
    
    sql_stmt << "DELETE FROM posts WHERE uuid = \"" << uuid << "\"";

    rV = sqlite3_exec ( this->db, 
                        sql_stmt.str().c_str(), 
                        NULL, 
                        NULL, 
                        &errmsg );

    if ( rV != SQLITE_OK )
    {
        cout << "sqlite error! error number " << rV << endl;
    }
    else
    {
        cout << "sqlite ok!" << endl;
    }
    sqlite3_close( this->db );
}

void rpl_storage::update_metric ( string uuid )
{
    int rV;
    char *errmsg;

    cout << __FUNCTION__ << " do some shit here with uuid " << uuid  << endl;
    stringstream sql_stmt;
    
    rV = sqlite3_open( this->db_location(), &this->db );
    if ( rV )
    {
        fprintf( stderr, "Couldn't open db %s\n", 
            this->db_location() );
        return;
    }

    sql_stmt << "UPDATE posts SET upvotes = upvotes + 1 WHERE uuid = \"" << uuid << "\"";

    rV = sqlite3_exec ( this->db, 
                        sql_stmt.str().c_str(), 
                        NULL, 
                        NULL, 
                        &errmsg );

    if ( rV != SQLITE_OK )
    {
        cout << "sqlite error! error number " << rV << endl;
    }
    else
    {
        cout << "sqlite ok!" << endl;
    }
    sqlite3_close( this->db );
}
