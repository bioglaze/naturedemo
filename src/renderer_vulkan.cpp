// This is an independent project of an individual developer. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "window.hpp"

#define _DEBUG 1
#define VK_CHECK( x ) { VkResult res = (x); assert( res == VK_SUCCESS ); }

struct SwapchainResource
{
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
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

struct Ubo
{
    VkBuffer ubo = VK_NULL_HANDLE;
    VkDeviceMemory uboMemory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo uboDesc = {};
    uint8_t* uboData = nullptr;
};

Ubo ubo;
VkDevice gDevice;
VkPhysicalDevice gPhysicalDevice;
VkInstance gInstance;
VkDebugUtilsMessengerEXT gDbgMessenger;
uint32_t gGraphicsQueueNodeIndex = 0;
SwapchainResource gSwapchainResources[ 3 ] = {};
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

static const char* getObjectType( VkObjectType type )
{
    switch( type )
    {
    case VK_OBJECT_TYPE_QUERY_POOL: return "VK_OBJECT_TYPE_QUERY_POOL";
    case VK_OBJECT_TYPE_OBJECT_TABLE_NVX: return "VK_OBJECT_TYPE_OBJECT_TABLE_NVX";
    case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION: return "VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION";
    case VK_OBJECT_TYPE_SEMAPHORE: return "VK_OBJECT_TYPE_SEMAPHORE";
    case VK_OBJECT_TYPE_SHADER_MODULE: return "VK_OBJECT_TYPE_SHADER_MODULE";
    case VK_OBJECT_TYPE_SWAPCHAIN_KHR: return "VK_OBJECT_TYPE_SWAPCHAIN_KHR";
    case VK_OBJECT_TYPE_SAMPLER: return "VK_OBJECT_TYPE_SAMPLER";
    case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX: return "VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX";
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

static bool CreateInstance( VkInstance& outInstance )
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
    const char* validationLayerNames[] = { "VK_LAYER_LUNARG_standard_validation" };
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
    VkResult result = vkCreateInstance( &instanceCreateInfo, nullptr, &outInstance );

    if (result != VK_SUCCESS)
    {
        printf( "Unable to create instance!\n" );
        return false;
    }

#if _DEBUG
    PFN_vkCreateDebugUtilsMessengerEXT dbgCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr( outInstance, "vkCreateDebugUtilsMessengerEXT" );
    VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info = {};
    dbg_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbg_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbg_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbg_messenger_create_info.pfnUserCallback = dbgFunc;
    result = dbgCreateDebugUtilsMessenger( outInstance, &dbg_messenger_create_info, nullptr, &gDbgMessenger );

    CmdBeginDebugUtilsLabelEXT = ( PFN_vkCmdBeginDebugUtilsLabelEXT )vkGetInstanceProcAddr( outInstance, "vkCmdBeginDebugUtilsLabelEXT" );
    CmdEndDebugUtilsLabelEXT = ( PFN_vkCmdEndDebugUtilsLabelEXT )vkGetInstanceProcAddr( outInstance, "vkCmdEndDebugUtilsLabelEXT" );
#endif
    assert( result == VK_SUCCESS );
    return result == VK_SUCCESS;
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

    if (presentQueueNodeIndex == UINT32_MAX)
    {
        for (uint32_t q = 0; q < queueCount; ++q)
        {
            if (supportsPresent[ q ] == VK_TRUE)
            {
                presentQueueNodeIndex = q;
                break;
            }
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

    if (gSwapchainImageCount == 0 || gSwapchainImageCount > 3)
    {
        printf( "Invalid count of swapchain images!\n ");
        return false;
    }

    VkImage images[ 3 ];
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

        gSwapchainResources[ i ].image = images[ i ];

        SetImageLayout( gSwapchainResources[ 0 ].drawCommandBuffer, gSwapchainResources[ i ].image,
                        VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1, 0, 1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
    }

    return true;
}

static void CreateRenderPassMSAA()
{
	VkAttachmentDescription attachments[ 3 ] = {};

    attachments[ 0 ].format = gColorFormat;
    attachments[ 0 ].samples = gMsaaSampleBits;
    attachments[ 0 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[ 0 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[ 0 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[ 0 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[ 0 ].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[ 0 ].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments[ 1 ].format = gDepthFormat;
    attachments[ 1 ].samples = gMsaaSampleBits;
    attachments[ 1 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[ 1 ].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[ 1 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[ 1 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[ 1 ].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[ 1 ].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    attachments[ 2 ].format = gColorFormat;
    attachments[ 2 ].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[ 2 ].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[ 2 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[ 2 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[ 2 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[ 2 ].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[ 2 ].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    constexpr VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    constexpr VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    constexpr VkAttachmentReference colorResolveReference = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;
    subpass.pResolveAttachments = &colorResolveReference;
    subpass.pDepthStencilAttachment = &depthReference;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 3;
    renderPassInfo.pAttachments = &attachments[ 0 ];
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkResult err = vkCreateRenderPass( gDevice, &renderPassInfo, nullptr, &gRenderPass );
    assert( err == VK_SUCCESS );
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
    commandBufferAllocateInfo.commandBufferCount = 3;

    VkCommandBuffer drawCmdBuffers[ 3 ];
    VK_CHECK( vkAllocateCommandBuffers( gDevice, &commandBufferAllocateInfo, drawCmdBuffers ) );

    for (uint32_t i = 0; i < 3; ++i)
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

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = uboSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VK_CHECK( vkCreateBuffer( gDevice, &bufferInfo, nullptr, &ubo.ubo ) );

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements( gDevice, ubo.ubo, &memReqs );

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, gDeviceMemoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
    VK_CHECK( vkAllocateMemory( gDevice, &allocInfo, nullptr, &ubo.uboMemory ) );

    VK_CHECK( vkBindBufferMemory( gDevice, ubo.ubo, ubo.uboMemory, 0 ) );

    ubo.uboDesc.buffer = ubo.ubo;
    ubo.uboDesc.offset = 0;
    ubo.uboDesc.range = uboSize;
    
    VK_CHECK( vkMapMemory( gDevice, ubo.uboMemory, 0, uboSize, 0, (void **)&ubo.uboData ) );
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
        
    for (int i = 0; i < 4; ++i)
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

void aeInitRenderer( unsigned width, unsigned height, struct xcb_connection_t* connection, unsigned window )
{
    CreateInstance( gInstance );
    CreateDevice();
    LoadFunctionPointers();
    CreateCommandBuffers();
    CreateSwapchain( width, height, 1, connection, window );
    CreateRenderPassMSAA();
}

    
