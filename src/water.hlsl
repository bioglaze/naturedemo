#if !VULKAN
#define layout(a,b)  
#endif

struct UniformData
{
    matrix localToClip;
    float4 color;
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

float wave( int i, float x, float y )
{
    const float wavelength[ 8 ] = { 2, 1, 3, 1, 4, 1, 5, 1 };
    const float speed[ 8 ] = { 1, 1, 1, 1, 1, 1, 1, 1 };
    const float amplitude[ 8 ] = { 3, 1, 2, 4, 1, 2, 1, 3 };
    const float direction[ 8 ] = { 1, 1, 1, 1, 1, 1, 1, 1 };

    float frequency = 2 * 3.14159265f / wavelength[i];
    float phase = speed[i] * frequency;
    float theta = dot(direction[i], float2(x, y));
    return amplitude[i] * sin(theta * frequency + pushConstants.timeSecs * phase);
}

float waveHeight(float x, float y)
{
    float height = 0.0;
    for (int i = 0; i < 8; ++i)
        height += wave(i, x, y);
    return height;
}

// Plane dimension is ~8 in x and z dimension. Vertices are ~1 units apart.
VSOutput mainVS( uint vertexId : SV_VertexID )
{
    VSOutput vsOut;
    vsOut.uv = uvs[ vertexId ] + float2( pushConstants.timeSecs, 0 );
    float4 pos = float4( positions[ vertexId ], 1 );
    //pos.y += sin( fmod(pos.x, 4.2f) * pushConstants.timeSecs * 15 ) / 2;
    //pos.y += sin( fmod(pos.z, 4.2f) * pushConstants.timeSecs * 15 ) / 2;
    //pos.y += sin( pos.x * 1000) * 2;//sin( fmod(pos.z, 2.2f) * pushConstants.timeSecs * 50 );
    //pos.y += waveHeight( fmod( positions[ vertexId ].x, 2.0f ), fmod( positions[ vertexId ].z, 2.0f ) );
    //pos.y += fmod( pos.x, 2.0f ) * 4;

    vsOut.pos = mul( data[ pushConstants.uboIndex ].localToClip, pos );
    vsOut.surfaceToCamera = -vsOut.pos.xyz;

    return vsOut;
}

float4 mainFS( VSOutput vsOut ) : SV_Target
{
    float3 normal = normalize( textures[ pushConstants.texture2Index ].Sample( sLinear, vsOut.uv ).xyz );
    float diffuse = max( dot( normal, data[ pushConstants.uboIndex ].lightDir ), 0.0f );

    float shininess = 0.2f;
    float3 surfaceToLight = float3( 0.5f, 0.5f, 0 );
    float specular = pow( max( 0.0, dot( vsOut.surfaceToCamera, reflect( -surfaceToLight, normal ) ) ), shininess );

    return textures[ pushConstants.texture1Index ].Sample( sLinear, vsOut.uv ) * diffuse + specular;
}
