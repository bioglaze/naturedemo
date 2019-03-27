// This is an independent project of an individual developer. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "shader.hpp"
#include <assert.h>
#include <stdio.h>
#include <vulkan/vulkan.h>
#include "file.hpp"

void SetObjectName( VkDevice device, uint64_t object, VkObjectType objectType, const char* name );
extern VkDevice gDevice;

struct aeShaderImpl
{
    VkPipelineShaderStageCreateInfo vertexInfo = {};
    VkPipelineShaderStageCreateInfo fragmentInfo = {};
    VkPipelineShaderStageCreateInfo computeInfo = {};
    VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
    VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
    VkShaderModule computeShaderModule = VK_NULL_HANDLE;
};

aeShaderImpl shaders[ 40 ];
int shaderCount = 0;

void DestroyShaders()
{
    for (int i = 0; i < shaderCount; ++i)
    {
        vkDestroyShaderModule( gDevice, shaders[ i ].vertexShaderModule, nullptr );
        vkDestroyShaderModule( gDevice, shaders[ i ].fragmentShaderModule, nullptr );
        vkDestroyShaderModule( gDevice, shaders[ i ].computeShaderModule, nullptr );
    }
}

void aeShaderGetInfo( aeShader& shader, VkPipelineShaderStageCreateInfo& outVertexInfo, VkPipelineShaderStageCreateInfo& outFragmentInfo )
{
    outVertexInfo = shaders[ shader.index ].vertexInfo;
    outFragmentInfo = shaders[ shader.index ].fragmentInfo;
}

aeShader aeCreateShader( const struct aeFile& vertexFile, const struct aeFile& fragmentFile )
{
    assert( shaderCount < 40 );

    aeShader outShader;
    outShader.index = shaderCount++;

    {
        VkShaderModuleCreateInfo moduleCreateInfo = {};
        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = vertexFile.size;
        moduleCreateInfo.pCode = (const uint32_t*)vertexFile.data;

        VkResult err = vkCreateShaderModule( gDevice, &moduleCreateInfo, nullptr, &shaders[ outShader.index ].vertexShaderModule );
        assert( err == VK_SUCCESS );
        SetObjectName( gDevice, (uint64_t)shaders[ outShader.index ].vertexShaderModule, VK_OBJECT_TYPE_SHADER_MODULE, vertexFile.path );

        shaders[ outShader.index ].vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaders[ outShader.index ].vertexInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaders[ outShader.index ].vertexInfo.module = shaders[ outShader.index ].vertexShaderModule;
        shaders[ outShader.index ].vertexInfo.pName = "mainVS";
    }

    {
        VkShaderModuleCreateInfo moduleCreateInfo = {};
        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = fragmentFile.size;
        moduleCreateInfo.pCode = (const uint32_t*)fragmentFile.data;

        VkResult err = vkCreateShaderModule( gDevice, &moduleCreateInfo, nullptr, &shaders[ outShader.index ].fragmentShaderModule );
        assert( err == VK_SUCCESS );
        SetObjectName( gDevice, (uint64_t)shaders[ outShader.index ].fragmentShaderModule, VK_OBJECT_TYPE_SHADER_MODULE, fragmentFile.path );

        shaders[ outShader.index ].fragmentInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaders[ outShader.index ].fragmentInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaders[ outShader.index ].fragmentInfo.module = shaders[ outShader.index ].fragmentShaderModule;
        shaders[ outShader.index ].fragmentInfo.pName = "mainFS";
    }

    return outShader;
}

aeShader aeCreateComputeShader( const aeFile& file, const char* name )
{
    assert( shaderCount < 40 );

    aeShader outShader;
    outShader.index = shaderCount++;

    VkShaderModuleCreateInfo moduleCreateInfo = {};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = file.size;
    moduleCreateInfo.pCode = ( const uint32_t* )file.data;

    VkResult err = vkCreateShaderModule( gDevice, &moduleCreateInfo, nullptr, &shaders[ outShader.index ].computeShaderModule );
    assert( err == VK_SUCCESS );
    SetObjectName( gDevice, ( uint64_t )shaders[ outShader.index ].computeShaderModule, VK_OBJECT_TYPE_SHADER_MODULE, file.path );

    shaders[ outShader.index ].computeInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaders[ outShader.index ].computeInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaders[ outShader.index ].computeInfo.module = shaders[ outShader.index ].computeShaderModule;
    shaders[ outShader.index ].computeInfo.pName = name;

    return outShader;
}
