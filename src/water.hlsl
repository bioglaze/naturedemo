#if !VULKAN
#define layout(a,b)  
#endif

struct UniformData
{
    matrix localToClip;
    float3 color;
};

layout(set=0, binding=0) Texture2D<float4> textures[ 80 ] : register(t0);
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
} pushConstants;

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

VSOutput waterVS( uint vertexId : SV_VertexID )
{
    VSOutput vsOut;
    vsOut.uv = uvs[ vertexId ];
    vsOut.pos = mul( data[ pushConstants.uboIndex ].localToClip, float4( positions[ vertexId ], 1 ) );
    
    vsOut.uv.y = 1.0f - vsOut.uv.y;

    return vsOut;
}

float4 waterFS( VSOutput vsOut ) : SV_Target
{
    return float4( 1, 0, 0, 1 );
    //return textures[ 0 ].Sample( sLinear, vsOut.uv );
}
