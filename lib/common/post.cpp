#include "rpl.h"
#include "rpl_storage.h"
#include <stdio.h>
extern "C"
{
#ifdef WIN32
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif

std::string Post::gen_uuid(){
#ifdef WIN32
        UUID uuid;
        int i;
        UuidCreate ( &uuid );

        unsigned char * str;
        UuidToStringA ( &uuid, &str );
            
        //TODO Not safe. Just hack tastic    
        for(i=0; str[i] != '\0'; i++)
            str[i] = toupper(str[i]);

        std::string s( ( char* ) str );
        
        RpcStringFreeA ( &str );
#else
        uuid_t uuid;
        uuid_generate_random ( uuid );
        char s[37];
        uuid_unparse ( uuid, s );
#endif
        return s;
    }
}

