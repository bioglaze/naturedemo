// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "matrix.hpp"
#include "mesh.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "vertexbuffer.hpp"
#include "vec3.hpp"
#include "window.hpp"

#define _DEBUG 1
#define VK_CHECK( x ) { VkResult res = (x); assert( res == VK_SUCCESS ); }

void aeShaderGetInfo( const aeShader& shader, VkPipelineShaderStageCreateInfo& outVertexInfo, VkPipelineShaderStageCreateInfo& outFragmentInfo );
VkBuffer VertexBufferGet( const VertexBuffer& buffer );
VkBufferView VertexBufferGetView( const VertexBuffer& buffer );
static void UpdateAndBindDescriptors( const VkBufferView& positionView, const VkBufferView& uvView );

struct DepthStencil
{
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
};

struct SwapchainResource
{
    DepthStencil depthStencil;
    DepthStencil depthResolve;
    VkImage image = VK_NULL_HANDLE;
    VkImage msaaColorImage = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkImageView msaaColorView = VK_NULL_HANDLE;
    VkDeviceMemory msaaColorMem = VK_NULL_HANDLE;
    VkFramebuffer frameBuffer = VK_NULL_HANDLE;
    VkCommandBuffer drawCommandBuffer = VK_NULL_HANDLE;
    VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
    VkSemaphore imageAcquiredSemaphore = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    static const unsigned SetCount = 100;
    VkDescriptorSet descriptorSets[ SetCount ];
    unsigned setIndex = 0;
};

struct UboStruct
{
    Matrix localToClip;
    Matrix localToView;
    float color[ 4 ];
    Vec3 lightDir;
};

struct Ubo
{
    VkBuffer ubo = VK_NULL_HANDLE;
    VkDeviceMemory uboMemory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo uboDesc = {};
    UboStruct* uboData = nullptr;
} ubos[ 4 ];

struct PSO
{
    VkPipeline pso = VK_NULL_HANDLE;
    BlendMode blendMode = BlendMode::Off;
    CullMode cullMode = CullMode::Off;
    DepthMode depthMode = DepthMode::NoneWriteOff;
    VkShaderModule vertexModule = VK_NULL_HANDLE;
    VkShaderModule fragmentModule = VK_NULL_HANDLE;
    FillMode fillMode = FillMode::Solid;
    Topology topology = Topology::Triangles;
};

PSO gPsos[ 250 ];

VkDevice gDevice;
VkPhysicalDevice gPhysicalDevice;
VkInstance gInstance;
VkDebugUtilsMessengerEXT gDbgMessenger;
uint32_t gGraphicsQueueNodeIndex = 0;
SwapchainResource gSwapchainResources[ 4 ] = {};
VkCommandPool gCmdPool;
VkPhysicalDeviceProperties gProperties = {};
VkPhysicalDeviceFeatures gFeatures = {};
VkPhysicalDeviceMemoryProperties gDeviceMemoryProperties;
VkFormat gColorFormat = VK_FORMAT_UNDEFINED;
VkFormat gDepthFormat = VK_FORMAT_UNDEFINED;
VkQueue gGraphicsQueue;
VkSurfaceKHR gSurface = VK_NULL_HANDLE;
VkSwapchainKHR gSwapchain = VK_NULL_HANDLE;
uint32_t gSwapchainImageCount = 0;
constexpr VkSampleCountFlagBits gMsaaSampleBits = VK_SAMPLE_COUNT_4_BIT;
VkRenderPass gRenderPass = VK_NULL_HANDLE;
unsigned gFrameIndex = 0;
unsigned gCurrentBuffer = 0;
VkCommandBuffer gCurrentDrawCommandBuffer;
constexpr unsigned TextureCount = 20;
VkSampler sampler;
VkImageView views[ TextureCount ];
VkPipelineLayout gPipelineLayout = VK_NULL_HANDLE;
VkCommandBuffer gTexCommandBuffer;
VkDescriptorSetLayout gDescriptorSetLayout;
VkDescriptorPool gDescriptorPool;
VkPipelineCache gPipelineCache = VK_NULL_HANDLE;

unsigned gWidth = 0;
unsigned gHeight = 0;

PFN_vkCreateSwapchainKHR createSwapchainKHR;
PFN_vkGetSwapchainImagesKHR getSwapchainImagesKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR getPhysicalDeviceSurfacePresentModesKHR;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR getPhysicalDeviceSurfaceSupportKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR getPhysicalDeviceSurfaceFormatsKHR;
PFN_vkAcquireNextImageKHR acquireNextImageKHR;
PFN_vkQueuePresentKHR queuePresentKHR;
PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT;
PFN_vkCmdBeginDebugUtilsLabelEXT CmdBeginDebugUtilsLabelEXT;
PFN_vkCmdEndDebugUtilsLabelEXT CmdEndDebugUtilsLabelEXT;

static void UpdateUBO( const Matrix& localToClip, const Matrix& localToView, const Vec3& lightDir, unsigned uboIndex )
{
    UboStruct ubo;
    ubo.localToClip = localToClip;
    ubo.localToView = localToView;
    ubo.lightDir = lightDir;

    memcpy( &ubos[ 0 ].uboData[ uboIndex ], &ubo, sizeof( UboStruct ) );
}

static const char* getObjectType( VkObjectType type )
{
    switch( type )
    {
    case VK_OBJECT_TYPE_QUERY_POOL: return "VK_OBJECT_TYPE_QUERY_POOL";
    case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION: return "VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION";
    case VK_OBJECT_TYPE_SEMAPHORE: return "VK_OBJECT_TYPE_SEMAPHORE";
    case VK_OBJECT_TYPE_SHADER_MODULE: return "VK_OBJECT_TYPE_SHADER_MODULE";
    case VK_OBJECT_TYPE_SWAPCHAIN_KHR: return "VK_OBJECT_TYPE_SWAPCHAIN_KHR";
    case VK_OBJECT_TYPE_SAMPLER: return "VK_OBJECT_TYPE_SAMPLER";
    case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT: return "VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT";
    case VK_OBJECT_TYPE_IMAGE: return "VK_OBJECT_TYPE_IMAGE";
    case VK_OBJECT_TYPE_UNKNOWN: return "VK_OBJECT_TYPE_UNKNOWN";
    case VK_OBJECT_TYPE_DESCRIPTOR_POOL: return "VK_OBJECT_TYPE_DESCRIPTOR_POOL";
    case VK_OBJECT_TYPE_COMMAND_BUFFER: return "VK_OBJECT_TYPE_COMMAND_BUFFER";
    case VK_OBJECT_TYPE_BUFFER: return "VK_OBJECT_TYPE_BUFFER";
    case VK_OBJECT_TYPE_SURFACE_KHR: return "VK_OBJECT_TYPE_SURFACE_KHR";
    case VK_OBJECT_TYPE_INSTANCE: return "VK_OBJECT_TYPE_INSTANCE";
    case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT: return "VK_OBJECT_TYPE_VALIDATION_CACHE_EXT";
    case VK_OBJECT_TYPE_IMAGE_VIEW: return "VK_OBJECT_TYPE_IMAGE_VIEW";
    case VK_OBJECT_TYPE_DESCRIPTOR_SET: return "VK_OBJECT_TYPE_DESCRIPTOR_SET";
    case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT: return "VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT";
    case VK_OBJECT_TYPE_COMMAND_POOL: return "VK_OBJECT_TYPE_COMMAND_POOL";
    case VK_OBJECT_TYPE_PHYSICAL_DEVICE: return "VK_OBJECT_TYPE_PHYSICAL_DEVICE";
    case VK_OBJECT_TYPE_DISPLAY_KHR: return "VK_OBJECT_TYPE_DISPLAY_KHR";
    case VK_OBJECT_TYPE_BUFFER_VIEW: return "VK_OBJECT_TYPE_BUFFER_VIEW";
    case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT: return "VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT";
    case VK_OBJECT_TYPE_FRAMEBUFFER: return "VK_OBJECT_TYPE_FRAMEBUFFER";
    case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE: return "VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE";
    case VK_OBJECT_TYPE_PIPELINE_CACHE: return "VK_OBJECT_TYPE_PIPELINE_CACHE";
    case VK_OBJECT_TYPE_PIPELINE_LAYOUT: return "VK_OBJECT_TYPE_PIPELINE_LAYOUT";
    case VK_OBJECT_TYPE_DEVICE_MEMORY: return "VK_OBJECT_TYPE_DEVICE_MEMORY";
    case VK_OBJECT_TYPE_FENCE: return "VK_OBJECT_TYPE_FENCE";
    case VK_OBJECT_TYPE_QUEUE: return "VK_OBJECT_TYPE_QUEUE";
    case VK_OBJECT_TYPE_DEVICE: return "VK_OBJECT_TYPE_DEVICE";
    case VK_OBJECT_TYPE_RENDER_PASS: return "VK_OBJECT_TYPE_RENDER_PASS";
    case VK_OBJECT_TYPE_DISPLAY_MODE_KHR: return "VK_OBJECT_TYPE_DISPLAY_MODE_KHR";
    case VK_OBJECT_TYPE_EVENT: return "VK_OBJECT_TYPE_EVENT";
    case VK_OBJECT_TYPE_PIPELINE: return "VK_OBJECT_TYPE_PIPELINE";
    default: return "unhandled type";
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL dbgFunc( VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity, VkDebugUtilsMessageTypeFlagsEXT msgType,
                                        const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* /*userData*/ )
{
    if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        printf( "ERROR: %s\n", callbackData->pMessage );
    }
    else if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        printf( "WARNING: %s\n", callbackData->pMessage );
    }
    else if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        printf( "INFO: %s\n", callbackData->pMessage );
    }
    else if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        printf( "VERBOSE: %s\n", callbackData->pMessage );
    }

    if (msgType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
    {
        printf( "GENERAL: %s\n", callbackData->pMessage );
    }
    else if (msgType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
    {
        printf( "PERF: %s\n", callbackData->pMessage );
    }

    if (callbackData->objectCount > 0)
    {
        printf( "Objects: %u\n", callbackData->objectCount );

        for (uint32_t i = 0; i < callbackData->objectCount; ++i)
        {
            const char* name = callbackData->pObjects[ i ].pObjectName ? callbackData->pObjects[ i ].pObjectName : "unnamed";
            printf( "Object %u: name: %s, type: %s\n", i, name, getObjectType( callbackData->pObjects[ i ].objectType ) );
        }
    }
    
    assert( !"Vulkan debug message" );

    return VK_FALSE;
}

void SetObjectName( VkDevice device, uint64_t object, VkObjectType objectType, const char* name )
{
    if (setDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = objectType;
        nameInfo.objectHandle = object;
        nameInfo.pObjectName = name;
        setDebugUtilsObjectNameEXT( device, &nameInfo );
    }
}

uint32_t GetMemoryType( uint32_t typeBits, const VkPhysicalDeviceMemoryProperties& deviceMemoryProperties, VkFlags properties )
{
    for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; ++i)
    {
        if ((typeBits & (1 << i)) != 0 && (deviceMemoryProperties.memoryTypes[ i ].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    assert( !"could not get memory type" );
    return 0;
}

void SetImageLayout( VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout,
                     VkImageLayout newImageLayout, unsigned layerCount, unsigned mipLevel, unsigned mipLevelCount, VkPipelineStageFlags srcStageFlags )
{
    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange.aspectMask = aspectMask;
    imageMemoryBarrier.subresourceRange.baseMipLevel = mipLevel;
    imageMemoryBarrier.subresourceRange.levelCount = mipLevelCount;
    imageMemoryBarrier.subresourceRange.layerCount = layerCount;

    switch (oldImageLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        imageMemoryBarrier.srcAccessMask = 0;
        break;
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        break;
    }

    if (newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }

    if (newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = imageMemoryBarrier.srcAccessMask | VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }

    if (newImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    if (newImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    if (newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        if (imageMemoryBarrier.srcAccessMask == 0)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }

        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }

    if (newImageLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    }

    VkPipelineStageFlags destStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    if (imageMemoryBarrier.dstAccessMask == VK_ACCESS_TRANSFER_WRITE_BIT)
    {
        destStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
        
    if (imageMemoryBarrier.dstAccessMask == VK_ACCESS_SHADER_READ_BIT)
    {
        destStageFlags = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    if (imageMemoryBarrier.dstAccessMask == VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
    {
        destStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }

    if (imageMemoryBarrier.dstAccessMask == VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
    {
        destStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }

    if (imageMemoryBarrier.dstAccessMask == VK_ACCESS_TRANSFER_READ_BIT)
    {
        destStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }

    vkCmdPipelineBarrier( cmdbuffer, srcStageFlags, destStageFlags, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier );
}

static VkPipeline CreatePipeline( const aeShader& shader, BlendMode blendMode, CullMode cullMode, DepthMode depthMode, FillMode fillMode, Topology topology )
{
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = topology == Topology::Triangles ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

    VkPipelineRasterizationStateCreateInfo rasterizationState = {};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.polygonMode = fillMode == FillMode::Solid ? VK_POLYGON_MODE_FILL : VK_POLYGON_MODE_LINE;

    if (cullMode == CullMode::Off)
    {
        rasterizationState.cullMode = VK_CULL_MODE_NONE;
    }
    else if (cullMode == CullMode::Back)
    {
        rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    }
    else if (cullMode == CullMode::Front)
    {
        rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
    }
    else
    {
        assert( false && "unhandled cull mode" );
    }

    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.lineWidth = 1;

    VkPipelineColorBlendAttachmentState blendAttachmentState[ 1 ] = {};
    blendAttachmentState[ 0 ].colorWriteMask = 0xF;
    blendAttachmentState[ 0 ].blendEnable = blendMode != BlendMode::Off ? VK_TRUE : VK_FALSE;
    blendAttachmentState[ 0 ].alphaBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentState[ 0 ].colorBlendOp = VK_BLEND_OP_ADD;

    if (blendMode == BlendMode::AlphaBlend)
    {
        blendAttachmentState[ 0 ].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachmentState[ 0 ].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState[ 0 ].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachmentState[ 0 ].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    }
    else if (blendMode == BlendMode::Additive)
    {
        blendAttachmentState[ 0 ].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachmentState[ 0 ].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachmentState[ 0 ].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachmentState[ 0 ].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    }

    VkPipelineColorBlendStateCreateInfo colorBlendState = {};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = blendAttachmentState;

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    VkDynamicState dynamicStateEnables[ 2 ] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = &dynamicStateEnables[ 0 ];
    dynamicState.dynamicStateCount = 2;

    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = depthMode == DepthMode::NoneWriteOff ? VK_FALSE : VK_TRUE;
    depthStencilState.depthWriteEnable = depthMode == DepthMode::LessOrEqualWriteOn ? VK_TRUE : VK_FALSE;

    if (depthMode == DepthMode::LessOrEqualWriteOn || depthMode == DepthMode::LessOrEqualWriteOff)
    {
        depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    }
    else if (depthMode == DepthMode::NoneWriteOff)
    {
        depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    }
    else
    {
        assert( false && "unhandled depth function" );
    }

    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
    depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
    depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.front = depthStencilState.back;

    VkPipelineMultisampleStateCreateInfo multisampleState = {};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = gMsaaSampleBits;

    VkPipelineShaderStageCreateInfo vertexInfo, fragmentInfo;
    aeShaderGetInfo( shader, vertexInfo, fragmentInfo );
    const VkPipelineShaderStageCreateInfo shaderStages[ 2 ] = { vertexInfo, fragmentInfo };

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineVertexInputStateCreateInfo inputState = {};
    inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = gPipelineLayout;
    pipelineCreateInfo.renderPass = gRenderPass;
    pipelineCreateInfo.pVertexInputState = &inputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pDynamicState = &dynamicState;

    VkPipeline pso;

    VK_CHECK( vkCreateGraphicsPipelines( gDevice, gPipelineCache, 1, &pipelineCreateInfo, nullptr, &pso ) );
    return pso;
}

static int GetPSO( const aeShader& shader, BlendMode blendMode, CullMode cullMode, DepthMode depthMode, FillMode fillMode, Topology topology )
{
    int psoIndex = -1;

    VkPipelineShaderStageCreateInfo vertexInfo, fragmentInfo;
    aeShaderGetInfo( shader, vertexInfo, fragmentInfo );

    for (int i = 0; i < 250; ++i)
    {
        if (gPsos[ i ].blendMode == blendMode && gPsos[ i ].depthMode == depthMode && gPsos[ i ].cullMode == cullMode &&
            gPsos[ i ].topology == topology && gPsos[ i ].fillMode == fillMode &&
            gPsos[ i ].vertexModule == vertexInfo.module && gPsos[ i ].fragmentModule == fragmentInfo.module)
        {
            psoIndex = i;
            break;
        }
    }

    if (psoIndex == -1)
    {
        int nextFreePsoIndex = -1;

        for (int i = 0; i < 250; ++i)
        {
            if (gPsos[ i ].pso == VK_NULL_HANDLE)
            {
                nextFreePsoIndex = i;
            }
        }

        assert( nextFreePsoIndex != -1 );

        psoIndex = nextFreePsoIndex;
        gPsos[ psoIndex ].pso = CreatePipeline( shader, blendMode, cullMode, depthMode, fillMode, topology );
        gPsos[ psoIndex ].blendMode = blendMode;
        gPsos[ psoIndex ].fillMode = fillMode;
        gPsos[ psoIndex ].topology = topology;
        gPsos[ psoIndex ].cullMode = cullMode;
        gPsos[ psoIndex ].depthMode = depthMode;
        gPsos[ psoIndex ].vertexModule = vertexInfo.module;
        gPsos[ psoIndex ].fragmentModule = fragmentInfo.module;
    }

    return psoIndex;
}

void aeRenderMesh( const aeMesh& mesh, const aeShader& shader, const Matrix& localToClip, const Matrix& localToView, const aeTexture2D& texture, const aeTexture2D& texture2, const Vec3& lightDir, unsigned uboIndex )
{
    UpdateUBO( localToClip, localToView, lightDir, uboIndex );
	UpdateAndBindDescriptors( VertexBufferGetView( GetPositions( mesh ) ), VertexBufferGetView( GetUVs( mesh ) ) );

    gSwapchainResources[ gCurrentBuffer ].setIndex = (gSwapchainResources[ gCurrentBuffer ].setIndex + 1) % gSwapchainResources[ gCurrentBuffer ].SetCount;

    const VertexBuffer& indices = GetIndices( mesh );
	vkCmdBindPipeline( gSwapchainResources[ gCurrentBuffer ].drawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gPsos[ GetPSO( shader, BlendMode::Off, CullMode::Back, DepthMode::LessOrEqualWriteOn, FillMode::Solid, Topology::Triangles ) ].pso );
	vkCmdBindIndexBuffer( gSwapchainResources[ gCurrentBuffer ].drawCommandBuffer, VertexBufferGet( indices ), 0, VK_INDEX_TYPE_UINT16 );

    static float timeSecs = 0;
    timeSecs += 0.0005f;

    struct PushConstants
    {
        unsigned uboIndex;
        unsigned texture1Index;
        unsigned texture2Index;
        float timeSecs;
    };
    
    PushConstants pushConstants = { uboIndex, (unsigned)texture.index, (unsigned)texture2.index, timeSecs };
    
	vkCmdPushConstants( gSwapchainResources[ gCurrentBuffer ].drawCommandBuffer, gPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof( pushConstants ), &pushConstants );

    vkCmdDrawIndexed( gSwapchainResources[ gCurrentBuffer ].drawCommandBuffer, indices.count, 1, 0, 0, 0 );
}

static void CreateInstance( VkInstance& outInstance )
{
    typedef VkResult(VKAPI_PTR * FuncPtrEnumerateInstanceVersion)(uint32_t * pApiVersion);
    FuncPtrEnumerateInstanceVersion vulkan11EnumerateInstanceVersion = (FuncPtrEnumerateInstanceVersion)vkGetInstanceProcAddr( VK_NULL_HANDLE, "vkEnumerateInstanceVersion" );

    uint32_t apiVersion = 0;
    const bool isVulkan11 = vulkan11EnumerateInstanceVersion && vulkan11EnumerateInstanceVersion( &apiVersion ) == VK_SUCCESS && VK_MAKE_VERSION( 1, 1, 0 ) <= apiVersion;
    
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "NatureDemo";
    appInfo.pEngineName = "NatureDemo";
    appInfo.apiVersion = isVulkan11 ? VK_API_VERSION_1_1 : VK_API_VERSION_1_0;

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, nullptr );
	assert( extensionCount < 20 );
    VkExtensionProperties extensions[ 20 ];
    vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, extensions );
    bool hasDebugUtils = false;

    for (uint32_t i = 0; i < extensionCount; ++i)
    {
        hasDebugUtils |= strcmp( extensions[ i ].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME ) == 0;
    }

    const char* enabledExtensions[] =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if VK_USE_PLATFORM_WIN32_KHR
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#if VK_USE_PLATFORM_XCB_KHR
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = hasDebugUtils ? 3 : 2;
    instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions;
#if _DEBUG
    const char* validationLayerNames[] = { "VK_LAYER_KHRONOS_validation" };
    instanceCreateInfo.enabledLayerCount = 1;
    instanceCreateInfo.ppEnabledLayerNames = validationLayerNames;
#endif
#if 0
    const VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT, VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT };
    VkValidationFeaturesEXT features = {};
    features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    features.enabledValidationFeatureCount = 2;
    features.pEnabledValidationFeatures = enables;

    instanceCreateInfo.pNext = &features;
#endif
	VK_CHECK( vkCreateInstance( &instanceCreateInfo, nullptr, &outInstance ) );

#if _DEBUG
    PFN_vkCreateDebugUtilsMessengerEXT dbgCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr( outInstance, "vkCreateDebugUtilsMessengerEXT" );
    VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info = {};
    dbg_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbg_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbg_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbg_messenger_create_info.pfnUserCallback = dbgFunc;
	VK_CHECK( dbgCreateDebugUtilsMessenger( outInstance, &dbg_messenger_create_info, nullptr, &gDbgMessenger ) );

    CmdBeginDebugUtilsLabelEXT = ( PFN_vkCmdBeginDebugUtilsLabelEXT )vkGetInstanceProcAddr( outInstance, "vkCmdBeginDebugUtilsLabelEXT" );
    CmdEndDebugUtilsLabelEXT = ( PFN_vkCmdEndDebugUtilsLabelEXT )vkGetInstanceProcAddr( outInstance, "vkCmdEndDebugUtilsLabelEXT" );
#endif
}

static void LoadFunctionPointers()
{
    createSwapchainKHR = (PFN_vkCreateSwapchainKHR)vkGetDeviceProcAddr( gDevice, "vkCreateSwapchainKHR" );
    getPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr( gInstance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR" );
    getPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr( gInstance, "vkGetPhysicalDeviceSurfacePresentModesKHR" );
    getPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr( gInstance, "vkGetPhysicalDeviceSurfaceFormatsKHR" );
    getPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr( gInstance, "vkGetPhysicalDeviceSurfaceSupportKHR" );
    getSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)vkGetDeviceProcAddr( gDevice, "vkGetSwapchainImagesKHR" );
    acquireNextImageKHR = (PFN_vkAcquireNextImageKHR)vkGetDeviceProcAddr( gDevice, "vkAcquireNextImageKHR" );
    queuePresentKHR = (PFN_vkQueuePresentKHR)vkGetDeviceProcAddr( gDevice, "vkQueuePresentKHR" );
    setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr( gInstance, "vkSetDebugUtilsObjectNameEXT" );
}

static bool CreateSwapchain( unsigned& width, unsigned& height, int presentInterval, struct xcb_connection_t* connection, unsigned window )
{
#if VK_USE_PLATFORM_WIN32_KHR
    (void)window;
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance = GetModuleHandle( nullptr );
    surfaceCreateInfo.hwnd = (HWND)connection;
    VkResult err = vkCreateWin32SurfaceKHR( gInstance, &surfaceCreateInfo, nullptr, &gSurface );
    assert( err == VK_SUCCESS );
#elif VK_USE_PLATFORM_XCB_KHR
    VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.connection = connection;
    surfaceCreateInfo.window = window;
    VkResult err = vkCreateXcbSurfaceKHR( gInstance, &surfaceCreateInfo, nullptr, &gSurface );
    assert( err == VK_SUCCESS );
#endif

    uint32_t queueCount;
    vkGetPhysicalDeviceQueueFamilyProperties( gPhysicalDevice, &queueCount, nullptr );
    assert( queueCount > 0 && queueCount < 5 && "None or more queues than buffers have elements! Increase element count." );

    VkQueueFamilyProperties queueProps[ 5 ];
    vkGetPhysicalDeviceQueueFamilyProperties( gPhysicalDevice, &queueCount, &queueProps[ 0 ] );

    VkBool32 supportsPresent[ 5 ];

    for (uint32_t i = 0; i < queueCount; ++i)
    {
        getPhysicalDeviceSurfaceSupportKHR( gPhysicalDevice, i, gSurface, &supportsPresent[ i ] );
    }

    uint32_t presentQueueNodeIndex = UINT32_MAX;

    for (uint32_t i = 0; i < queueCount; ++i)
    {
        if ((queueProps[ i ].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            if (gGraphicsQueueNodeIndex == UINT32_MAX)
            {
                gGraphicsQueueNodeIndex = i;
            }

            if (supportsPresent[ i ] == VK_TRUE)
            {
                gGraphicsQueueNodeIndex = i;
                presentQueueNodeIndex = i;
                break;
            }
        }
    }

    for (uint32_t q = 0; q < queueCount && presentQueueNodeIndex == UINT32_MAX; ++q)
    {
        if (supportsPresent[ q ] == VK_TRUE)
        {
            presentQueueNodeIndex = q;
            break;
        }
    }

    if (gGraphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)
    {
        printf( "graphics or present queue not found!\n" );
        return false;
    }

    if (gGraphicsQueueNodeIndex != presentQueueNodeIndex)
    {
        printf( "graphics and present queues must have the same index!\n" );
        return false;
    }

    uint32_t formatCount = 0;
    err = getPhysicalDeviceSurfaceFormatsKHR( gPhysicalDevice, gSurface, &formatCount, nullptr );
    assert( err == VK_SUCCESS && formatCount > 0 && formatCount < 20 && "Invalid format count" );
        
    VkSurfaceFormatKHR surfFormats[ 20 ];        
    err = getPhysicalDeviceSurfaceFormatsKHR( gPhysicalDevice, gSurface, &formatCount, surfFormats );
    assert( err == VK_SUCCESS && formatCount < 20 && "Too many formats!" );
    
    bool foundSRGB = false;

    for (uint32_t formatIndex = 0; formatIndex < formatCount; ++formatIndex)
    {
        if (surfFormats[ formatIndex ].format == VK_FORMAT_B8G8R8A8_SRGB || surfFormats[ formatIndex ].format == VK_FORMAT_R8G8B8A8_SRGB)
        {
            gColorFormat = surfFormats[ formatIndex ].format;
            foundSRGB = true;
        }
    }

    if (!foundSRGB && formatCount == 1 && surfFormats[ 0 ].format == VK_FORMAT_UNDEFINED)
    {
        gColorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else if (!foundSRGB)
    {
        gColorFormat = surfFormats[ 0 ].format;
    }

    VkSurfaceCapabilitiesKHR surfCaps;
    VK_CHECK( getPhysicalDeviceSurfaceCapabilitiesKHR( gPhysicalDevice, gSurface, &surfCaps ) );

    uint32_t presentModeCount = 0;
    VK_CHECK( getPhysicalDeviceSurfacePresentModesKHR( gPhysicalDevice, gSurface, &presentModeCount, nullptr ) );

    if (presentModeCount == 0 || presentModeCount > 10)
    {
        printf( "Invalid present mode count.\n" );
        return false;
    }
    
    VkPresentModeKHR presentModes[ 10 ];
        
    VK_CHECK( getPhysicalDeviceSurfacePresentModesKHR( gPhysicalDevice, gSurface, &presentModeCount, presentModes ) );

    VkExtent2D swapchainExtent;
    if (surfCaps.currentExtent.width == (uint32_t)-1)
    {
		swapchainExtent = { (uint32_t)width, (uint32_t)height };
    }
    else
    {
        swapchainExtent = surfCaps.currentExtent;
        width = surfCaps.currentExtent.width;
        height = surfCaps.currentExtent.height;
    }

    const uint32_t desiredNumberOfSwapchainImages = ((surfCaps.maxImageCount > 0) && (surfCaps.minImageCount + 1 > surfCaps.maxImageCount)) ? surfCaps.maxImageCount : surfCaps.minImageCount + 1;
    
    VkSurfaceTransformFlagsKHR preTransform = (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : surfCaps.currentTransform;

    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = gSurface;
    swapchainInfo.minImageCount = desiredNumberOfSwapchainImages;
    swapchainInfo.imageFormat = gColorFormat;
    swapchainInfo.imageColorSpace = surfFormats[ 0 ].colorSpace;
    swapchainInfo.imageExtent = { swapchainExtent.width, swapchainExtent.height };
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 0;
    swapchainInfo.presentMode = presentInterval == 0 ? VK_PRESENT_MODE_IMMEDIATE_KHR : VK_PRESENT_MODE_FIFO_KHR;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    VK_CHECK( createSwapchainKHR( gDevice, &swapchainInfo, nullptr, &gSwapchain ) );
	SetObjectName( gDevice, (uint64_t)gSwapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "swap chain" );

    VK_CHECK( getSwapchainImagesKHR( gDevice, gSwapchain, &gSwapchainImageCount, nullptr ) );

    if (gSwapchainImageCount == 0 || gSwapchainImageCount > 4)
    {
        printf( "Invalid count of swapchain images!\n ");
        return false;
    }

    VkImage images[ 4 ] = {};
    VK_CHECK( getSwapchainImagesKHR( gDevice, gSwapchain, &gSwapchainImageCount, images ) );

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK( vkBeginCommandBuffer( gSwapchainResources[ 0 ].drawCommandBuffer, &cmdBufInfo ) );

    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        VkImageViewCreateInfo colorAttachmentView = {};
        colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorAttachmentView.format = gColorFormat;
        colorAttachmentView.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorAttachmentView.subresourceRange.levelCount = 1;
        colorAttachmentView.subresourceRange.layerCount = 1;
        colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorAttachmentView.image = images[ i ];
        VK_CHECK( vkCreateImageView( gDevice, &colorAttachmentView, nullptr, &gSwapchainResources[ i ].view ) );
        SetObjectName( gDevice, (uint64_t)gSwapchainResources[ i ].view, VK_OBJECT_TYPE_IMAGE_VIEW, "swap chain view" );

        gSwapchainResources[ i ].image = images[ i ];

        SetImageLayout( gSwapchainResources[ 0 ].drawCommandBuffer, gSwapchainResources[ i ].image,
                        VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1, 0, 1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
    }

    return true;
}

static void CreateRenderPassMSAA()
{
	VkAttachmentDescription attachments[ 4 ] = {};

    attachments[ 0 ].format = gColorFormat;
    attachments[ 0 ].samples = gMsaaSampleBits;
    attachments[ 0 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[ 0 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[ 0 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[ 0 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[ 0 ].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[ 0 ].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments[ 1 ].format = gColorFormat;
    attachments[ 1 ].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[ 1 ].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[ 1 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[ 1 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[ 1 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[ 1 ].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[ 1 ].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachments[ 2 ].format = gDepthFormat;
    attachments[ 2 ].samples = gMsaaSampleBits;
    attachments[ 2 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[ 2 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[ 2 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[ 2 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[ 2 ].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[ 2 ].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    attachments[ 3 ].format = gDepthFormat;
    attachments[ 3 ].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[ 3 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[ 3 ].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[ 3 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[ 3 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[ 3 ].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[ 3 ].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = {};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depthReference = {};
    depthReference.attachment = 2;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    // Two resolve attachment references for color and depth
    VkAttachmentReference resolveReferences[ 2 ] = {};
    resolveReferences[ 0 ].attachment = 1;
    resolveReferences[ 0 ].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resolveReferences[ 1 ].attachment = 3;
    resolveReferences[ 1 ].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;
    // Pass our resolve attachments to the sub pass
    subpass.pResolveAttachments = &resolveReferences[ 0 ];
    subpass.pDepthStencilAttachment = &depthReference;
    
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 4;
    renderPassInfo.pAttachments = &attachments[ 0 ];
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    
    VK_CHECK( vkCreateRenderPass( gDevice, &renderPassInfo, nullptr, &gRenderPass ) );
}

static void CreateCommandBuffers()
{
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = gGraphicsQueueNodeIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK( vkCreateCommandPool( gDevice, &cmdPoolInfo, nullptr, &gCmdPool ) );

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = gCmdPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 4;

    VkCommandBuffer drawCmdBuffers[ 4 ];
    VK_CHECK( vkAllocateCommandBuffers( gDevice, &commandBufferAllocateInfo, drawCmdBuffers ) );

    for (uint32_t i = 0; i < 4; ++i)
    {
        gSwapchainResources[ i ].drawCommandBuffer = drawCmdBuffers[ i ];
        const char* name = "drawCommandBuffer 0";
        if (i == 1)
        {
            name = "drawCommandBuffer 1";
        }
        else if (i == 2)
        {
            name = "drawCommandBuffer 2";
        }
        
        SetObjectName( gDevice, (uint64_t)drawCmdBuffers[ i ], VK_OBJECT_TYPE_COMMAND_BUFFER, name );
            
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VK_CHECK( vkCreateSemaphore( gDevice, &semaphoreCreateInfo, nullptr, &gSwapchainResources[ i ].imageAcquiredSemaphore ) );
        SetObjectName( gDevice, (uint64_t)gSwapchainResources[ i ].imageAcquiredSemaphore, VK_OBJECT_TYPE_SEMAPHORE, "imageAcquiredSemaphore" );

        VK_CHECK( vkCreateSemaphore( gDevice, &semaphoreCreateInfo, nullptr, &gSwapchainResources[ i ].renderCompleteSemaphore ) );
        SetObjectName( gDevice, (uint64_t)gSwapchainResources[ i ].imageAcquiredSemaphore, VK_OBJECT_TYPE_SEMAPHORE, "renderCompleteSemaphore" );

        VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };

        VK_CHECK( vkCreateFence( gDevice, &fenceCreateInfo, nullptr, &gSwapchainResources[ i ].fence ) );
    }
}

static void CreateUBO()
{
    constexpr VkDeviceSize uboSize = 256 * 3 + 80 * 64;

    for (unsigned i = 0; i < 4; ++i)
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = uboSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        VK_CHECK( vkCreateBuffer( gDevice, &bufferInfo, nullptr, &ubos[ i ].ubo ) );

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements( gDevice, ubos[ i ].ubo, &memReqs );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, gDeviceMemoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
        VK_CHECK( vkAllocateMemory( gDevice, &allocInfo, nullptr, &ubos[ i ].uboMemory ) );

        VK_CHECK( vkBindBufferMemory( gDevice, ubos[ i ].ubo, ubos[ i ].uboMemory, 0 ) );

        ubos[ i ].uboDesc.buffer = ubos[ i ].ubo;
        ubos[ i ].uboDesc.offset = 0;
        ubos[ i ].uboDesc.range = uboSize;
    
        VK_CHECK( vkMapMemory( gDevice, ubos[ i ].uboMemory, 0, uboSize, 0, (void **)&ubos[ i ].uboData ) );
    }
}

static bool CreateDevice()
{
    uint32_t gpuCount = 0;
    VK_CHECK( vkEnumeratePhysicalDevices( gInstance, &gpuCount, nullptr ) );
    VK_CHECK( vkEnumeratePhysicalDevices( gInstance, &gpuCount, &gPhysicalDevice ) );

    vkGetPhysicalDeviceProperties( gPhysicalDevice, &gProperties );
    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( gPhysicalDevice, &queueCount, nullptr );
    assert( queueCount < 20 && "More queues than the array has elements!" );

    VkQueueFamilyProperties queueProps[ 20 ];
    vkGetPhysicalDeviceQueueFamilyProperties( gPhysicalDevice, &queueCount, queueProps );

    uint32_t graphicsQueueIndex = 0;

    for (graphicsQueueIndex = 0; graphicsQueueIndex < queueCount; ++graphicsQueueIndex)
    {
        if (queueProps[ graphicsQueueIndex ].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            break;
        }
    }

    float queuePriorities = 0;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriorities;

    const char* enabledExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    vkGetPhysicalDeviceFeatures( gPhysicalDevice, &gFeatures );

    VkPhysicalDeviceFeatures enabledFeatures = {};
    enabledFeatures.shaderClipDistance = gFeatures.shaderClipDistance;
    enabledFeatures.shaderCullDistance = gFeatures.shaderCullDistance;
    enabledFeatures.textureCompressionBC = gFeatures.textureCompressionBC;
    enabledFeatures.fillModeNonSolid = gFeatures.fillModeNonSolid;
    enabledFeatures.samplerAnisotropy = gFeatures.samplerAnisotropy;
    enabledFeatures.fragmentStoresAndAtomics = gFeatures.fragmentStoresAndAtomics;
    enabledFeatures.shaderStorageImageExtendedFormats = gFeatures.shaderStorageImageExtendedFormats;
    enabledFeatures.multiDrawIndirect = gFeatures.multiDrawIndirect;
    
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions;

    VK_CHECK( vkCreateDevice( gPhysicalDevice, &deviceCreateInfo, nullptr, &gDevice ) );

    vkGetPhysicalDeviceMemoryProperties( gPhysicalDevice, &gDeviceMemoryProperties );
    vkGetDeviceQueue( gDevice, graphicsQueueIndex, 0, &gGraphicsQueue );

    const VkFormat depthFormats[ 4 ] = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM };
        
    for (unsigned i = 0; i < 4; ++i)
    {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties( gPhysicalDevice, depthFormats[ i ], &formatProps );

        if ((formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) && gDepthFormat == VK_FORMAT_UNDEFINED)
        {
            gDepthFormat = depthFormats[ i ];
        }
    }

    assert( gDepthFormat != VK_FORMAT_UNDEFINED && "undefined depth format!" );

    CreateUBO();
    
    return gDepthFormat != VK_FORMAT_UNDEFINED;
}

static void CreateFramebufferMSAA( int width, int height )
{
    VkImageView attachments[ 4 ] = {};

    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.renderPass = gRenderPass;
    frameBufferCreateInfo.attachmentCount = 4;
    frameBufferCreateInfo.pAttachments = attachments;
    frameBufferCreateInfo.width = width;
    frameBufferCreateInfo.height = height;
    frameBufferCreateInfo.layers = 1;

    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        VkImageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = gColorFormat;
        info.extent = { (uint32_t)width, (uint32_t)height, 1 };
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.samples = gMsaaSampleBits;
        info.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VK_CHECK( vkCreateImage( gDevice, &info, nullptr, &gSwapchainResources[ i ].msaaColorImage ) );
        SetObjectName( gDevice, (uint64_t)gSwapchainResources[ i ].msaaColorImage, VK_OBJECT_TYPE_IMAGE, "msaaColorImage" );

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements( gDevice, gSwapchainResources[ i ].msaaColorImage, &memReqs );
        VkMemoryAllocateInfo memAlloc = {};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, gDeviceMemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

        VK_CHECK( vkAllocateMemory( gDevice, &memAlloc, nullptr, &gSwapchainResources[ i ].msaaColorMem ) );

        vkBindImageMemory( gDevice, gSwapchainResources[ i ].msaaColorImage, gSwapchainResources[ i ].msaaColorMem, 0 );

        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = gSwapchainResources[ i ].msaaColorImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = gColorFormat;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        VK_CHECK( vkCreateImageView( gDevice, &viewInfo, nullptr, &gSwapchainResources[ i ].msaaColorView ) );
        SetObjectName( gDevice, (uint64_t)gSwapchainResources[ i ].msaaColorView, VK_OBJECT_TYPE_IMAGE_VIEW, "msaaColorView" );

        attachments[ 0 ] = gSwapchainResources[ i ].msaaColorView;
        attachments[ 1 ] = gSwapchainResources[ i ].view;
        attachments[ 2 ] = gSwapchainResources[ i ].depthStencil.view;
        attachments[ 3 ] = gSwapchainResources[ i ].depthResolve.view;

        VK_CHECK( vkCreateFramebuffer( gDevice, &frameBufferCreateInfo, nullptr, &gSwapchainResources[ i ].frameBuffer ) );
        SetObjectName( gDevice, (uint64_t)gSwapchainResources[ i ].frameBuffer, VK_OBJECT_TYPE_FRAMEBUFFER, "framebuffer" );
    }
}

static void CreateDepthStencil( uint32_t width, uint32_t height )
{
    VkImageCreateInfo image = {};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = gDepthFormat;
    image.extent = { width, height, 1 };
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = gMsaaSampleBits;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    for (unsigned i = 0; i < 4; ++i)
    {
        VK_CHECK( vkCreateImage( gDevice, &image, nullptr, &gSwapchainResources[ i ].depthStencil.image ) );
        SetObjectName( gDevice, (uint64_t)gSwapchainResources[ i ].depthStencil.image, VK_OBJECT_TYPE_IMAGE, "depthStencil" );

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements( gDevice, gSwapchainResources[ i ].depthStencil.image, &memReqs );

        VkMemoryAllocateInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_alloc.allocationSize = memReqs.size;
        mem_alloc.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, gDeviceMemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
        VK_CHECK( vkAllocateMemory( gDevice, &mem_alloc, nullptr, &gSwapchainResources[ i ].depthStencil.mem ) );

        VK_CHECK( vkBindImageMemory( gDevice, gSwapchainResources[ i ].depthStencil.image, gSwapchainResources[ i ].depthStencil.mem, 0 ) );
        SetImageLayout( gSwapchainResources[ 0 ].drawCommandBuffer, gSwapchainResources[ i ].depthStencil.image, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 0, 1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );

        VkImageViewCreateInfo depthStencilView = {};
        depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthStencilView.format = gDepthFormat;
        depthStencilView.subresourceRange = {};
        depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        depthStencilView.subresourceRange.baseMipLevel = 0;
        depthStencilView.subresourceRange.levelCount = 1;
        depthStencilView.subresourceRange.baseArrayLayer = 0;
        depthStencilView.subresourceRange.layerCount = 1;
        depthStencilView.image = gSwapchainResources[ i ].depthStencil.image;
        VK_CHECK( vkCreateImageView( gDevice, &depthStencilView, nullptr, &gSwapchainResources[ i ].depthStencil.view ) );
        SetObjectName( gDevice, (uint64_t)gSwapchainResources[ i ].depthStencil.view, VK_OBJECT_TYPE_IMAGE_VIEW, "depthStencil view" );

        // MSAA depth resolve:
        VkImageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = gDepthFormat;
        info.extent = { width, height, 1 };
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VK_CHECK( vkCreateImage( gDevice, &info, nullptr, &gSwapchainResources[ i ].depthResolve.image ) );
        SetObjectName( gDevice, (uint64_t)gSwapchainResources[ i ].depthResolve.image, VK_OBJECT_TYPE_IMAGE, "depthResolve" );

        vkGetImageMemoryRequirements( gDevice, gSwapchainResources[ i ].depthResolve.image, &memReqs );
        VkMemoryAllocateInfo memAlloc = {};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, gDeviceMemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

        VK_CHECK( vkAllocateMemory( gDevice, &memAlloc, nullptr, &gSwapchainResources[ i ].depthResolve.mem ) );
        SetObjectName( gDevice, (uint64_t)gSwapchainResources[ i ].depthResolve.mem, VK_OBJECT_TYPE_DEVICE_MEMORY, "depthResolve" );

        vkBindImageMemory( gDevice, gSwapchainResources[ i ].depthResolve.image, gSwapchainResources[ i ].depthResolve.mem, 0 );

        // Create image view for the MSAA target
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = gSwapchainResources[ i ].depthResolve.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = gDepthFormat;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        VK_CHECK( vkCreateImageView( gDevice, &viewInfo, nullptr, &gSwapchainResources[ i ].depthResolve.view ) );
    }
}

static void CreateDescriptorSets()
{
    VkDescriptorSetLayoutBinding layoutBindingImage = {};
    layoutBindingImage.binding = 0;
    layoutBindingImage.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    layoutBindingImage.descriptorCount = TextureCount;
    layoutBindingImage.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding layoutBindingSampler = {};
    layoutBindingSampler.binding = 1;
    layoutBindingSampler.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    layoutBindingSampler.descriptorCount = 1;
    layoutBindingSampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding layoutBindingBuffer = {};
    layoutBindingBuffer.binding = 2;
    layoutBindingBuffer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    layoutBindingBuffer.descriptorCount = 1;
    layoutBindingBuffer.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding layoutBindingBuffer2 = {};
    layoutBindingBuffer2.binding = 3;
    layoutBindingBuffer2.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindingBuffer2.descriptorCount = 1;
    layoutBindingBuffer2.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding layoutBindingBuffer3 = {};
    layoutBindingBuffer3.binding = 4;
    layoutBindingBuffer3.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    layoutBindingBuffer3.descriptorCount = 1;
    layoutBindingBuffer3.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    constexpr unsigned bindingCount = 5;
    VkDescriptorSetLayoutBinding bindings[ bindingCount ] = { layoutBindingImage, layoutBindingSampler, layoutBindingBuffer, layoutBindingBuffer2, layoutBindingBuffer3 };
        
    VkDescriptorSetLayoutCreateInfo setCreateInfo = {};
    setCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setCreateInfo.bindingCount = bindingCount;
    setCreateInfo.pBindings = bindings;

    VK_CHECK( vkCreateDescriptorSetLayout( gDevice, &setCreateInfo, nullptr, &gDescriptorSetLayout ) );

    VkDescriptorPoolSize typeCounts[ bindingCount ];
    typeCounts[ 0 ].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    typeCounts[ 0 ].descriptorCount = 4 * TextureCount * gSwapchainResources[ 0 ].SetCount;
    typeCounts[ 1 ].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    typeCounts[ 1 ].descriptorCount = 4 * gSwapchainResources[ 0 ].SetCount;
    typeCounts[ 2 ].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    typeCounts[ 2 ].descriptorCount = 4 * gSwapchainResources[ 0 ].SetCount;
    typeCounts[ 3 ].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    typeCounts[ 3 ].descriptorCount = 4 * gSwapchainResources[ 0 ].SetCount;
    typeCounts[ 4 ].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    typeCounts[ 4 ].descriptorCount = 4 * gSwapchainResources[ 0 ].SetCount;

    VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = bindingCount;
    descriptorPoolInfo.pPoolSizes = typeCounts;
    descriptorPoolInfo.maxSets = 4 * gSwapchainResources[ 0 ].SetCount;
    descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VK_CHECK( vkCreateDescriptorPool( gDevice, &descriptorPoolInfo, nullptr, &gDescriptorPool ) );

    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        for (uint32_t s = 0; s < gSwapchainResources[ 0 ].SetCount; ++s)
        {
            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = gDescriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &gDescriptorSetLayout;
            
            VK_CHECK( vkAllocateDescriptorSets( gDevice, &allocInfo, &gSwapchainResources[ i ].descriptorSets[ s ] ) );
        }
    }

    VkPipelineLayoutCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = 1;
    createInfo.pSetLayouts = &gDescriptorSetLayout;

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.size = sizeof( int ) * 4;

    createInfo.pushConstantRangeCount = 1;
    createInfo.pPushConstantRanges = &pushConstantRange;
    VK_CHECK( vkCreatePipelineLayout( gDevice, &createInfo, nullptr, &gPipelineLayout ) );
}

static void UpdateAndBindDescriptors( const VkBufferView& positionView, const VkBufferView& uvView )
{
    VkDescriptorImageInfo samplerInfos[ TextureCount ] = {};

    for (unsigned i = 0; i < TextureCount; ++i)
    {
        samplerInfos[ i ].sampler = sampler;
        samplerInfos[ i ].imageView = views[ i ];
        samplerInfos[ i ].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    const VkDescriptorSet& dstSet = gSwapchainResources[ gCurrentBuffer ].descriptorSets[ gSwapchainResources[ gCurrentBuffer ].setIndex ];

    VkWriteDescriptorSet imageSet = {};
    imageSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    imageSet.dstSet = dstSet;
    imageSet.descriptorCount = TextureCount;
    imageSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    imageSet.pImageInfo = &samplerInfos[ 0 ];
    imageSet.dstBinding = 0;

    VkWriteDescriptorSet samplerSet = {};
    samplerSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    samplerSet.dstSet = dstSet;
    samplerSet.descriptorCount = 1;
    samplerSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    samplerSet.pImageInfo = &samplerInfos[ 0 ];
    samplerSet.dstBinding = 1;

    VkWriteDescriptorSet bufferSet = {};
    bufferSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    bufferSet.dstSet = dstSet;
    bufferSet.descriptorCount = 1;
    bufferSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    bufferSet.pTexelBufferView = &positionView;
    bufferSet.dstBinding = 2;

    VkDescriptorBufferInfo uboDesc = {};
    uboDesc.buffer = ubos[ 0 ].ubo;
    uboDesc.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet bufferSet2 = {};
    bufferSet2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    bufferSet2.dstSet = dstSet;
    bufferSet2.descriptorCount = 1;
    bufferSet2.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bufferSet2.pBufferInfo = &uboDesc;
    bufferSet2.dstBinding = 3;

    VkWriteDescriptorSet bufferSet3 = {};
    bufferSet3.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    bufferSet3.dstSet = dstSet;
    bufferSet3.descriptorCount = 1;
    bufferSet3.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    bufferSet3.pTexelBufferView = &uvView;
    bufferSet3.dstBinding = 4;

    constexpr unsigned setCount = 5;
    VkWriteDescriptorSet sets[ setCount ] = { imageSet, samplerSet, bufferSet, bufferSet2, bufferSet3 };
    vkUpdateDescriptorSets( gDevice, setCount, sets, 0, nullptr );

    vkCmdBindDescriptorSets( gSwapchainResources[ gCurrentBuffer ].drawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        gPipelineLayout, 0, 1, &dstSet, 0, nullptr );
}

void CopyBuffer( VkBuffer source, VkBuffer& destination, unsigned bufferSize )
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

void aeInitRenderer( unsigned width, unsigned height, struct xcb_connection_t* connection, unsigned window )
{
    gWidth = width;
    gHeight = height;

    CreateInstance( gInstance );
    CreateDevice();
    LoadFunctionPointers();
    CreateCommandBuffers();
    CreateSwapchain( width, height, 1, connection, window );
    CreateDepthStencil( width, height );

    // Flush setup command buffer
    {
        VK_CHECK( vkEndCommandBuffer( gSwapchainResources[ 0 ].drawCommandBuffer ) );

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &gSwapchainResources[ 0 ].drawCommandBuffer;

        VK_CHECK( vkQueueSubmit( gGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE ) );
        VK_CHECK( vkQueueWaitIdle( gGraphicsQueue ) );
    }

    CreateRenderPassMSAA();
    CreateFramebufferMSAA( width, height );

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = gCmdPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VK_CHECK( vkAllocateCommandBuffers( gDevice, &commandBufferAllocateInfo, &gTexCommandBuffer ) );
    SetObjectName( gDevice, (uint64_t)gTexCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "texCommandBuffer" );

    CreateDescriptorSets();

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = samplerInfo.magFilter;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = samplerInfo.addressModeU;
    samplerInfo.addressModeW = samplerInfo.addressModeU;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    VK_CHECK( vkCreateSampler( gDevice, &samplerInfo, nullptr, &sampler ) );
    SetObjectName( gDevice, (uint64_t)sampler, VK_OBJECT_TYPE_SAMPLER, "sampler" );
}

void aeBeginRenderPass()
{
    VkClearValue clearValues[ 4 ] = {};
    
    clearValues[ 2 ].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = gRenderPass;
    renderPassBeginInfo.renderArea = { { 0, 0, }, { gWidth, gHeight } };
    renderPassBeginInfo.clearValueCount = gMsaaSampleBits != VK_SAMPLE_COUNT_1_BIT ? 4 : 2;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = gSwapchainResources[ gCurrentBuffer ].frameBuffer;

    vkCmdBeginRenderPass( gSwapchainResources[ gCurrentBuffer ].drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
}

void aeEndRenderPass()
{
    vkCmdEndRenderPass( gSwapchainResources[ gCurrentBuffer ].drawCommandBuffer );
}

void aeBeginFrame()
{
    vkWaitForFences( gDevice, 1, &gSwapchainResources[ gFrameIndex ].fence, VK_TRUE, UINT64_MAX );
    vkResetFences( gDevice, 1, &gSwapchainResources[ gFrameIndex ].fence );

    VkResult err = VK_SUCCESS;

    do
    {
        err = acquireNextImageKHR( gDevice, gSwapchain, UINT64_MAX, gSwapchainResources[ gFrameIndex ].imageAcquiredSemaphore, VK_NULL_HANDLE, &gCurrentBuffer );

        if (err == VK_ERROR_OUT_OF_DATE_KHR)
        {
            printf( "Swapchain is out of date!\n" );
            break;
        }
        else if (err == VK_SUBOPTIMAL_KHR)
        {
            printf( "Swapchain is suboptimal!\n" );
            break;
        }
        else
        {
            assert( err == VK_SUCCESS );
        }
    } while (err != VK_SUCCESS);

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufInfo.pNext = nullptr;

    VK_CHECK( vkBeginCommandBuffer( gSwapchainResources[ gCurrentBuffer ].drawCommandBuffer, &cmdBufInfo ) );
    gCurrentDrawCommandBuffer = gSwapchainResources[ gCurrentBuffer ].drawCommandBuffer;

    VkViewport viewport = { 0, 0, (float)gWidth, (float)gHeight, 0.0f, 1.0f };
	vkCmdSetViewport( gSwapchainResources[ gCurrentBuffer ].drawCommandBuffer, 0, 1, &viewport );

	VkRect2D scissor = { { 0, 0 }, { gWidth, gHeight } };
	vkCmdSetScissor( gSwapchainResources[ gCurrentBuffer ].drawCommandBuffer, 0, 1, &scissor );
}

void aeEndFrame()
{
    VK_CHECK( vkEndCommandBuffer( gSwapchainResources[ gCurrentBuffer ].drawCommandBuffer ) );

    VkPipelineStageFlags pipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &pipelineStages;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &gSwapchainResources[ gFrameIndex ].imageAcquiredSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &gSwapchainResources[ gFrameIndex ].renderCompleteSemaphore;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &gSwapchainResources[ gCurrentBuffer ].drawCommandBuffer;

    VK_CHECK( vkQueueSubmit( gGraphicsQueue, 1, &submitInfo, gSwapchainResources[ gFrameIndex ].fence ) );

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &gSwapchain;
    presentInfo.pImageIndices = &gCurrentBuffer;
    presentInfo.pWaitSemaphores = &gSwapchainResources[ gFrameIndex ].renderCompleteSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    VkResult err = queuePresentKHR( gGraphicsQueue, &presentInfo );

    if (err == VK_ERROR_OUT_OF_DATE_KHR)
    {
        printf( "Swapchain is out of date!\n" );
        // Handle resizing etc.
    }
    else if (err == VK_SUBOPTIMAL_KHR)
    {
        printf( "Swapchain is suboptimal!\n" );
    }
    else
    {
        assert( err == VK_SUCCESS );
    }

    gFrameIndex = (gFrameIndex + 1) % gSwapchainImageCount;
}
