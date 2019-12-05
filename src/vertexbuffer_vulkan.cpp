// This is an independent project of an individual developer. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "vertexbuffer.hpp"
#include <assert.h>
#include <string.h>
#include <vulkan/vulkan.h>

#define VK_CHECK( x ) { VkResult res = (x); assert( res == VK_SUCCESS ); }

extern VkDevice gDevice;
extern VkPhysicalDeviceMemoryProperties gDeviceMemoryProperties;
extern VkQueue gGraphicsQueue;
extern VkCommandPool gCmdPool;

// TODO remove!
extern VkBufferView gPositionsView;
extern VkBufferView gUVSView;

void SetObjectName( VkDevice device, uint64_t object, VkObjectType objectType, const char* name );
uint32_t GetMemoryType( uint32_t typeBits, const VkPhysicalDeviceMemoryProperties& deviceMemoryProperties, VkFlags properties );

struct VertexBufferImpl
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkBufferView view = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
};

VertexBufferImpl buffers[ 1000 ];
int bufferCount = 0;

void DeinitBuffers( VkDevice device )
{
    for( int i = 0; i < bufferCount; ++i )
    {
        vkDestroyBuffer( device, buffers[ i ].buffer, nullptr );
        vkDestroyBufferView( device, buffers[ i ].view, nullptr );
        vkDestroyBuffer( device, buffers[ i ].stagingBuffer, nullptr );
        vkFreeMemory( device, buffers[ i ].memory, nullptr );
        vkFreeMemory( device, buffers[ i ].stagingMemory, nullptr );
    }
}

void CopyBuffer( VkBuffer source, VkBuffer& destination, int bufferSize )
{
    VkCommandBufferAllocateInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufInfo.commandPool = gCmdPool;
    cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufInfo.commandBufferCount = 1;

    VkCommandBuffer copyCommandBuffer;
    VK_CHECK( vkAllocateCommandBuffers( gDevice, &cmdBufInfo, &copyCommandBuffer ) );

    VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkBufferCopy copyRegion = {};
    copyRegion.size = bufferSize;

    VK_CHECK( vkBeginCommandBuffer( copyCommandBuffer, &cmdBufferBeginInfo ) );

    vkCmdCopyBuffer( copyCommandBuffer, source, destination, 1, &copyRegion );

    VK_CHECK( vkEndCommandBuffer( copyCommandBuffer ) );

    VkSubmitInfo copySubmitInfo = {};
    copySubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    copySubmitInfo.commandBufferCount = 1;
    copySubmitInfo.pCommandBuffers = &copyCommandBuffer;

    VK_CHECK( vkQueueSubmit( gGraphicsQueue, 1, &copySubmitInfo, VK_NULL_HANDLE ) );
    VK_CHECK( vkQueueWaitIdle( gGraphicsQueue ) );
    vkFreeCommandBuffers( gDevice, cmdBufInfo.commandPool, 1, &copyCommandBuffer );
}

static void CreateBuffer( unsigned dataBytes, VkMemoryPropertyFlags memoryFlags, VkBufferUsageFlags usageFlags, const char* debugName, VkBuffer& outBuffer, VkDeviceMemory& outMemory )
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = dataBytes;
    bufferInfo.usage = usageFlags;
    VK_CHECK( vkCreateBuffer( gDevice, &bufferInfo, nullptr, &outBuffer ) );
    SetObjectName( gDevice, (uint64_t)outBuffer, VK_OBJECT_TYPE_BUFFER, debugName );

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements( gDevice, outBuffer, &memReqs );
    
    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, gDeviceMemoryProperties, memoryFlags );
    VK_CHECK( vkAllocateMemory( gDevice, &memAlloc, nullptr, &outMemory ) );

    VK_CHECK( vkBindBufferMemory( gDevice, outBuffer, outMemory, 0 ) );
}

VkBuffer VertexBufferGet( const VertexBuffer& buffer )
{
    return buffers[ buffer.index ].buffer;
}

VkBufferView VertexBufferGetView( const VertexBuffer& buffer )
{
    return buffers[ buffer.index ].view;
}

VertexBuffer CreateVertexBuffer( const void* data, unsigned dataBytes, BufferType bufferType, BufferUsage bufferUsage, const char* debugName )
{
    assert( dataBytes > 0 );

    VertexBuffer outBuffer;
    outBuffer.index = bufferCount++;

    CreateBuffer( dataBytes, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, "staging buffer", buffers[ outBuffer.index ].stagingBuffer, buffers[ outBuffer.index ].stagingMemory );

    void* bufferData = nullptr;
    VK_CHECK( vkMapMemory( gDevice, buffers[ outBuffer.index ].stagingMemory, 0, dataBytes, 0, &bufferData ) );

    memcpy( bufferData, data, dataBytes );
    vkUnmapMemory( gDevice, buffers[ outBuffer.index ].stagingMemory );

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    if (bufferUsage == BufferUsage::Index)
    {
        usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    
    CreateBuffer( dataBytes, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, usage, debugName, buffers[ outBuffer.index ].buffer, buffers[ outBuffer.index ].memory );
    CopyBuffer( buffers[ outBuffer.index ].stagingBuffer, buffers[ outBuffer.index ].buffer, dataBytes );

    VkBufferViewCreateInfo bufferViewInfo = {};
    bufferViewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    bufferViewInfo.buffer = buffers[ outBuffer.index ].buffer;
    bufferViewInfo.range = VK_WHOLE_SIZE;

    if (bufferType == BufferType::Uint)
    {
        bufferViewInfo.format = VK_FORMAT_R32_UINT;
        outBuffer.count = dataBytes / 4;
    }
    else if (bufferType == BufferType::Ushort)
    {
        bufferViewInfo.format = VK_FORMAT_R16_UINT;
        outBuffer.count = dataBytes / 2;
    }
    else if (bufferType == BufferType::Float2)
    {
        bufferViewInfo.format = VK_FORMAT_R32G32_SFLOAT;
        outBuffer.count = dataBytes / (2 * 4);
    }
    else if (bufferType == BufferType::Float3)
    {
        bufferViewInfo.format = VK_FORMAT_R32G32B32_SFLOAT;
        outBuffer.count = dataBytes / (3 * 4);
    }
    else if (bufferType == BufferType::Float4)
    {
        bufferViewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        outBuffer.count = dataBytes / (4 * 4);
    }
    else
    {
        assert( !"Unhandled data type!" );
    }

    if (bufferUsage == BufferUsage::Vertex)
    {
        VK_CHECK( vkCreateBufferView( gDevice, &bufferViewInfo, nullptr, &buffers[ outBuffer.index ].view ) );
        SetObjectName( gDevice, (uint64_t)buffers[ outBuffer.index ].view, VK_OBJECT_TYPE_BUFFER_VIEW, "buffer view" );

        // TODO remove
        if (bufferType == BufferType::Float3)
        {
            gPositionsView = buffers[ outBuffer.index ].view;
        }
        else if (bufferType == BufferType::Float2)
        {
            gUVSView = buffers[ outBuffer.index ].view;
        }
    }

    return outBuffer;
}
