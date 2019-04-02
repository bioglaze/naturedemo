// This is an independent project of an individual developer. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "texture.hpp"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "file.hpp"

struct aeTextureImpl
{
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    unsigned width = 0;
    unsigned height = 0;
    VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
};

aeTextureImpl textures[ 100 ];
int textureCount = 0;

static void LoadTGA( const aeFile& file, unsigned& outWidth, unsigned& outHeight, unsigned &outDataBeginOffset )
{
    unsigned char* data = (unsigned char*)file.data;

    unsigned offs = 0;
    //int idLength = data[ offs++ ];
    //int colorMapType = data[ offs++ ];
    offs++;
    offs++;
    int imageType = data[ offs++ ];

    if (imageType != 2 && imageType != 10)
    {
        printf( "Incompatible .tga file: %s\n", file.path );
    }

    offs += 5; // colorSpec
    offs += 4; // specBegin

    unsigned width = data[ offs ] | (data[ offs + 1 ] << 8);
    ++offs;

    unsigned height = data[ offs + 1 ] | (data[ offs + 2 ] << 8);
    ++offs;

    outWidth = width;
    outHeight = height;

    offs += 4; // specEnd
    outDataBeginOffset = offs;
}

static unsigned GetMipLevelCount( unsigned width, unsigned height )
{
    return (unsigned)floor( log2( fmax( width, height ) ) ) + 1;
}

aeTexture2D aeLoadTexture( const struct aeFile& file, unsigned flags )
{
    assert( textureCount < 100 );

    aeTexture2D outTexture;
    outTexture.index = textureCount++;
    aeTextureImpl& tex = textures[ outTexture.index ];

    int bytesPerPixel = 4;
    unsigned dataBeginOffset = 0;
    unsigned mipLevelCount = 1;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    bool isOpaque = false;
    unsigned mipOffsets[ 15 ] = {};

    if (strstr( file.path, ".tga" ) || strstr( file.path, ".TGA" ))
    {
        LoadTGA( file, tex.width, tex.height, dataBeginOffset );
        mipLevelCount = (flags & aeTextureFlags::GenerateMips) ? GetMipLevelCount( tex.width, tex.height ) : 1;
        assert( mipLevelCount <= 15 );
    }

    return outTexture;
}
