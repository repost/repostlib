#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>

#include "rpl.h"
#include "rpl_storage.h"

using namespace std;

const char rpl_storage::DATABASE_NAME[] = "repost";
const char rpl_storage::CREATE_POST_TABLE[] = 
    "CREATE TABLE IF NOT EXISTS posts (id INTEGER PRIMARY KEY, author TEXT, \
    uuid TEXT, content TEXT, upvotes INTEGER, downvotes INTEGER);";
bool initialised = false;
int iRowsReturned = 0;

rpl_storage *rpl_storage::INSTANCE = new rpl_storage();

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
    rc = sqlite3_open( rpl_storage::DATABASE_NAME, &this->db );
    if ( rc )
    {
        fprintf( stderr, "Couldn't open db %s\n", 
            rpl_storage::DATABASE_NAME );
    }
    else
    {
        this->initialised = true;
        this->setup_tables();
    }

#ifdef CLOSE_CONNECTION
    sqlite3_close( this->db );
#endif
    printf( "< rpl_storage\n" );
}

rpl_storage::~rpl_storage()
{
}

rpl_storage *rpl_storage::get_instance ()
{
    return INSTANCE;
}

void rpl_storage::add_link (Link &link)
{

}

void rpl_storage::add_post (Post *post)
{
    int rc;
    int rV;
    char *errmsg;
    string sql_stmt;

    printf( "> add_post\n" );
#ifdef CLOSE_CONNECTION
    rc = sqlite3_open( rpl_storage::DATABASE_NAME, &this->db );
    if ( rc )
    {
        fprintf( stderr, "Couldn't open db %s\n", 
            rpl_storage::DATABASE_NAME );
        return;
    }
#endif

    sql_stmt = "INSERT INTO posts ( uuid, content ) VALUES"
              "('" + post->uuid() + "', '" + post->content().c_str() + "')";

    printf ( "insert sql: %s\n", sql_stmt.c_str());
    rV = sqlite3_exec ( this->db, sql_stmt.c_str(), NULL, NULL, &errmsg );

    if ( errmsg != NULL )
    {
        printf( "error insert: %s\n", errmsg );
        sqlite3_free ( errmsg );
    }

#ifdef CLOSE_CONNECTION
    sqlite3_close( this->db );
#endif

    printf( "< add_post\n" );

}

void rpl_storage::get_link (Link *link)
{

}



int print_post (void * id, int columns, char **column_text, char **column_name)
{
    cout << ">" << __FUNCTION__ << endl;

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
        printf ( "%s = %s\n", column_name[i], column_text[i] );
        //cout << column_name[i] << " = " << column_text[i] << endl;
    }
    n++;
    iRowsReturned++;
    cout << "<" << __FUNCTION__ << endl;
    return 0;
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

#ifdef CLOSE_CONNECTION
    rc = sqlite3_open( rpl_storage::DATABASE_NAME, &this->db );
    if ( rc )
    {
        fprintf( stderr, "Couldn't open db %s\n", 
            rpl_storage::DATABASE_NAME );
        return;
    }
#endif

    sql_stmt << "SELECT * FROM posts LIMIT " << count << " OFFSET " << from; 

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

#ifdef CLOSE_CONNECTION
    sqlite3_close( this->db );
#endif

    cout << "< get_post" << endl;

    return iRowsReturned;
}

bool rpl_storage::setup_tables ()
{
    int rV;
    char *errmsg = NULL;

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
    }
    
    printf( "< %s\n", __FUNCTION__ );
}

