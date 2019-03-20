#pragma once

struct aeFile
{
    unsigned char* data = nullptr;
    unsigned size = 0;
    char path[ 260 ] = {};
};

aeFile aeLoadFile( const char* path );
