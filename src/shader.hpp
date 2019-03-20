#pragma once

struct aeShader
{
    int index = 0;
};

aeShader aeCreateShader( const struct aeFile& vertexFile, const aeFile& fragmentFile );
aeShader aeCreateComputeShader( const aeFile& file, const char* name );
