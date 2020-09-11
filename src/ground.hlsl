#if !VULKAN
#define layout(a,b)  
#endif

struct UniformData
{
    matrix localToClip;
    matrix localToView;
    float3 color;
    float3 lightDir;
};

layout(set=0, binding=0) Texture2D<float4> textures[ 20 ] : register(t0);
layout(set=0, binding=1) SamplerState sLinear : register(s0);
layout(set=0, binding=2) Buffer<float3> positions : register(b2);
layout(set=0, binding=3) cbuffer cbPerFrame : register(b3)
{
    UniformData data[ 1000 ];
};
layout(set=0, binding=4) Buffer<float2> uvs : register(b4);
layout(push_constant) cbuffer PushConstants
{
    int uboIndex;
    int texture1Index;
    int texture2Index;
    float timeSecs;
    float uvScale;
} pushConstants;

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

VSOutput mainVS( uint vertexId : SV_VertexID )
{
    VSOutput vsOut;
    vsOut.uv = uvs[ vertexId ];
    vsOut.pos = mul( data[ pushConstants.uboIndex ].localToClip, float4( positions[ vertexId ], 1 ) );
    
    vsOut.uv.y = 1.0f - vsOut.uv.y;

    return vsOut;
}

float4 mainFS( VSOutput vsOut ) : SV_Target
{
    return textures[ pushConstants.texture1Index ].Sample( sLinear, vsOut.uv );
}
