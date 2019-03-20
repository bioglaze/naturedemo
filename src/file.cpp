#include "file.hpp"
#include <stdio.h>
#include <string.h>

aeFile aeLoadFile( const char* path )
{
    aeFile outFile;
    FILE* file = fopen( path, "rb" );

    if (file)
    {    
        fseek( file, 0, SEEK_END );
        auto length = ftell( file );
        fseek( file, 0, SEEK_SET );
        outFile.data = new unsigned char[ length ];
        outFile.size = (unsigned)length;
        strcpy( outFile.path, path );
        fread( outFile.data, 1, length, file );    
        fclose( file );
    }
    else
    {
        printf( "Could not open file %s\n", path );
    }
    
    return outFile;
}
