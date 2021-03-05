struct UniformData
{
    matrix localToClip;
    matrix localToView;
    float4 color;
    float3 lightDir;
};

[[vk::binding(0)]] Texture2D<float4> textures[ 20 ];
[[vk::binding(1)]] SamplerState sLinear;
[[vk::binding(2)]] Buffer<float3> positions;
[[vk::binding(3)]] cbuffer cbPerFrame
{
    UniformData data[ 1000 ];
};
[[vk::binding(4)]] Buffer<float2> uvs;
struct PushConstants
{
    int uboIndex;
    int texture1Index;
    int texture2Index;
    float timeSecs;
    float uvScale;
};
[[vk::push_constant]] PushConstants pushConstants;
