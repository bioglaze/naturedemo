#if !VULKAN
#define layout(a,b)  
#endif

struct UniformData
{
    matrix localToClip;
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
} pushConstants;

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    float3 surfaceToCamera : TEXCOORD1;
};

VSOutput mainVS( uint vertexId : SV_VertexID )
{
    VSOutput vsOut;
    vsOut.uv = uvs[ vertexId ] * 2 + float2( pushConstants.timeSecs, 0 );
    vsOut.uv.y = vsOut.uv.y;
    vsOut.pos = mul( data[ pushConstants.uboIndex ].localToClip, float4( positions[ vertexId ], 1 ) );
    vsOut.pos.y += sin( positions[ vertexId ].x * pushConstants.timeSecs * 50 );
    vsOut.surfaceToCamera = -vsOut.pos.xyz;

    return vsOut;
}

float4 mainFS( VSOutput vsOut ) : SV_Target
{
    float3 normal = normalize( textures[ pushConstants.texture2Index ].Sample( sLinear, vsOut.uv ).xyz );
    float diffuse = max( dot( normal, float3(0, 1, 0)/*-data[ pushConstants.uboIndex ].lightDir*/ ), 0.0f );

    float shininess = 0.2f;
    float3 surfaceToLight = float3( 0.5f, 0.5f, 0 );
    float specular = pow( max( 0.0, dot( vsOut.surfaceToCamera, reflect( -surfaceToLight, normal ) ) ), shininess );

    return textures[ pushConstants.texture1Index ].Sample( sLinear, vsOut.uv ) * diffuse + specular;
}
