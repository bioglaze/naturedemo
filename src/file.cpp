// This is an independent project of an individual developer. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "file.hpp"
#include <stdio.h>
#include <string.h>

aeFile aeLoadFile( const char* path )
{
    aeFile outFile;
    FILE* file = nullptr;
    /*errno_t err =*/ fopen_s( &file, path, "rb" );

    if (file)
    {    
        fseek( file, 0, SEEK_END );
        auto length = ftell( file );
        fseek( file, 0, SEEK_SET );
        outFile.data = new unsigned char[ length ];
        outFile.size = (unsigned)length;
        strncpy_s( outFile.path, 260, path, strlen( path ) );
        fread( outFile.data, 1, length, file );    
        fclose( file );
    }
    else
    {
        printf( "Could not open file %s\n", path );
    }
    
    return outFile;
}
