#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "window.hpp"

VkDevice gDevice;
VkInstance gInstance;

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
    appInfo.pApplicationName = "ExperimentalEngine";
    appInfo.pEngineName = "ExperimentalEngine";
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
#if AE3D_OPENVR
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#endif
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
#if AE3D_OPENVR
    instanceCreateInfo.enabledExtensionCount = hasDebugUtils ? 4 : 3;
#else
    instanceCreateInfo.enabledExtensionCount = hasDebugUtils ? 3 : 2;
#endif
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
    result = dbgCreateDebugUtilsMessenger( outInstance, &dbg_messenger_create_info, nullptr, &renderers[ 0 ].dbgMessenger );

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

void aeInitRenderer( unsigned width, unsigned height )
{
    LoadFunctionPointers();
}

    
