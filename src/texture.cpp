// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "texture.hpp"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "file.hpp"

#define VK_CHECK( x ) { VkResult res = (x); assert( res == VK_SUCCESS ); }

extern VkImageView views[ 20 ];
extern VkDevice gDevice;
extern VkPhysicalDeviceMemoryProperties gDeviceMemoryProperties;
extern VkQueue gGraphicsQueue;
extern VkCommandPool gCmdPool;

void SetObjectName( VkDevice device, uint64_t object, VkObjectType objectType, const char* name );
uint32_t GetMemoryType( uint32_t typeBits, const VkPhysicalDeviceMemoryProperties& deviceMemoryProperties, VkFlags properties );
void SetImageLayout( VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout,
                     VkImageLayout newImageLayout, unsigned layerCount, unsigned mipLevel, unsigned mipLevelCount, VkPipelineStageFlags srcStageFlags );

struct aeTextureImpl
{
    VkImage image = VK_NULL_HANDLE;
    unsigned width = 0;
    unsigned height = 0;
    VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
};

static aeTextureImpl textures[ 100 ];
static int textureCount = 0;
static VkCommandBuffer texCommandBuffer;

static void LoadTGA( const aeFile& file, unsigned& outWidth, unsigned& outHeight, unsigned &outDataBeginOffset, unsigned& outBitsPerPixel )
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
    outBitsPerPixel = data[ offs - 2 ];

    outDataBeginOffset = offs;
}

static unsigned GetMipLevelCount( unsigned width, unsigned height )
{
    return (unsigned)floor( log2( fmax( width, height ) ) ) + 1;
}

aeTexture2D aeLoadTexture( const struct aeFile& file, unsigned flags )
{
    assert( textureCount < 100 );

    if (texCommandBuffer == VK_NULL_HANDLE)
    {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = gCmdPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;

        VK_CHECK( vkAllocateCommandBuffers( gDevice, &commandBufferAllocateInfo, &texCommandBuffer ) );
        SetObjectName( gDevice, (uint64_t)texCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "texCommandBuffer" );
    }
    
    aeTexture2D outTexture;
    outTexture.index = textureCount++;
    assert( outTexture.index < textureCount );
    aeTextureImpl& tex = textures[ outTexture.index ];

    int bytesPerPixel = 4;
    unsigned dataBeginOffset = 0;
    unsigned mipLevelCount = 1;
    VkFormat format = VK_FORMAT_B8G8R8A8_SRGB;

    if (strstr( file.path, ".tga" ) || strstr( file.path, ".TGA" ))
    {
        unsigned bitsPerPixel = 32;
        LoadTGA( file, tex.width, tex.height, dataBeginOffset, bitsPerPixel );
        bytesPerPixel = bitsPerPixel / 8;

        /*if (bytesPerPixel == 3)
        {
            format = VK_FORMAT_R8G8B8_SRGB;
            }*/
        
        mipLevelCount = (flags & aeTextureFlags::GenerateMips) ? GetMipLevelCount( tex.width, tex.height ) : 1;
        assert( mipLevelCount <= 15 );
    }
    else
    {
        assert( !"Only .tga is supported!" );
    }

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.mipLevels = mipLevelCount;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = { (uint32_t)tex.width, (uint32_t)tex.height, 1 };
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VK_CHECK( vkCreateImage( gDevice, &imageCreateInfo, nullptr, &tex.image ) );
    SetObjectName( gDevice, (uint64_t)tex.image, VK_OBJECT_TYPE_IMAGE, file.path );

    VkMemoryRequirements memReqs = {};
    vkGetImageMemoryRequirements( gDevice, tex.image, &memReqs );

    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, gDeviceMemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    VK_CHECK( vkAllocateMemory( gDevice, &memAllocInfo, nullptr, &tex.deviceMemory ) );
    VK_CHECK( vkBindImageMemory( gDevice, tex.image, tex.deviceMemory, 0 ) );

    VkBuffer stagingBuffer = VK_NULL_HANDLE;

    VkDeviceSize imageSize = tex.width * tex.height * bytesPerPixel;

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = imageSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK( vkCreateBuffer( gDevice, &bufferCreateInfo, nullptr, &stagingBuffer ) );

    vkGetBufferMemoryRequirements( gDevice, stagingBuffer, &memReqs );

    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, gDeviceMemoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    VK_CHECK( vkAllocateMemory( gDevice, &memAllocInfo, nullptr, &stagingMemory ) );

    VK_CHECK( vkBindBufferMemory( gDevice, stagingBuffer, stagingMemory, 0 ) );

    void* stagingData;
    VK_CHECK( vkMapMemory( gDevice, stagingMemory, 0, memReqs.size, 0, &stagingData ) );
    memcpy( stagingData, &file.data[ dataBeginOffset ], imageSize );

    VkMappedMemoryRange flushRange = {};
    flushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    flushRange.memory = stagingMemory;
    flushRange.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges( gDevice, 1, &flushRange );

    vkUnmapMemory( gDevice, stagingMemory );

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.levelCount = mipLevelCount;
    viewInfo.image = tex.image;
    VK_CHECK( vkCreateImageView( gDevice, &viewInfo, nullptr, &views[ outTexture.index ] ) );
    SetObjectName( gDevice, (uint64_t)views[ outTexture.index ], VK_OBJECT_TYPE_IMAGE_VIEW, file.path );

    static bool isFilled = false;

    if (!isFilled)
    {
        for (unsigned i = 0; i < 20; ++i)
        {
            views[ i ] = views[ outTexture.index ];
        }

        isFilled = true;
    }

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VK_CHECK( vkBeginCommandBuffer( texCommandBuffer, &cmdBufInfo ) );

    SetImageLayout( texCommandBuffer, tex.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, 0, 1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );

    VkBufferImageCopy bufferCopyRegion = {};
    bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferCopyRegion.imageSubresource.mipLevel = 0;
    bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
    bufferCopyRegion.imageSubresource.layerCount = 1;
    bufferCopyRegion.imageExtent.width = tex.width;
    bufferCopyRegion.imageExtent.height = tex.height;
    bufferCopyRegion.imageExtent.depth = 1;
    
    vkCmdCopyBufferToImage( texCommandBuffer, stagingBuffer, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion );

    vkEndCommandBuffer( texCommandBuffer );

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &texCommandBuffer;
    VK_CHECK( vkQueueSubmit( gGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE ) );
    
    vkDeviceWaitIdle( gDevice );

    VK_CHECK( vkBeginCommandBuffer( texCommandBuffer, &cmdBufInfo ) );

    SetImageLayout( texCommandBuffer, tex.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1, 0, 1, VK_PIPELINE_STAGE_TRANSFER_BIT );

    VkBuffer stagingBuffers[ 15 ] = {};
    VkDeviceMemory stagingMemories[ 15 ] = {};

    for (unsigned i = 1; i < mipLevelCount; ++i)
    {
        VkImageBlit imageBlit = {};
        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.srcSubresource.layerCount = 1;
        imageBlit.srcSubresource.mipLevel = 0;
        imageBlit.srcOffsets[ 0 ] = { 0, 0, 0 };
        imageBlit.srcOffsets[ 1 ] = { int32_t( tex.width ), int32_t( tex.height ), 1 };

        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.dstSubresource.baseArrayLayer = 0;
        imageBlit.dstSubresource.layerCount = 1;
        imageBlit.dstSubresource.mipLevel = i;
        imageBlit.dstOffsets[ 0 ] = { 0, 0, 0 };
        imageBlit.dstOffsets[ 1 ] = { int32_t( tex.width >> i ), int32_t( tex.height >> i ), 1 };

        SetImageLayout( texCommandBuffer, tex.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, i, 1, VK_PIPELINE_STAGE_TRANSFER_BIT );
        vkCmdBlitImage( texCommandBuffer, tex.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR );
        //SetImageLayout( texCommandBuffer, tex.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1, i, 1, VK_PIPELINE_STAGE_TRANSFER_BIT );
    }

    //SetImageLayout( texCommandBuffer, tex.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 0, mipLevelCount, VK_PIPELINE_STAGE_TRANSFER_BIT );
    
    vkEndCommandBuffer( texCommandBuffer );
    VK_CHECK( vkQueueSubmit( gGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE ) );

    vkDeviceWaitIdle( gDevice );

    VK_CHECK( vkBeginCommandBuffer( texCommandBuffer, &cmdBufInfo ) );

    for (unsigned i = 1; i < mipLevelCount; ++i)
    {
        SetImageLayout( texCommandBuffer, tex.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1, i, 1, VK_PIPELINE_STAGE_TRANSFER_BIT );
    }

    SetImageLayout( texCommandBuffer, tex.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 0, mipLevelCount, VK_PIPELINE_STAGE_TRANSFER_BIT );

    vkEndCommandBuffer( texCommandBuffer );
    VK_CHECK( vkQueueSubmit( gGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE ) );

    vkDeviceWaitIdle( gDevice );

    vkFreeMemory( gDevice, stagingMemory, nullptr );
    vkDestroyBuffer( gDevice, stagingBuffer, nullptr );

    for (unsigned i = 0; i < 15; ++i)
    {
        vkDestroyBuffer( gDevice, stagingBuffers[ i ], nullptr );
        vkFreeMemory( gDevice, stagingMemories[ i ], nullptr );
    }

    return outTexture;
}
