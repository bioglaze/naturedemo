#include "shader_common.h"

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    float2 uv2 : TEXCOORD1;
};

VSOutput mainVS( uint vertexId : SV_VertexID )
{
    VSOutput vsOut;
    vsOut.uv = uvs[ vertexId ] + float2(pushConstants.timeSecs, 0);
    vsOut.uv2 = uvs[ vertexId ] + float2(-pushConstants.timeSecs, -pushConstants.timeSecs);
    vsOut.pos = mul( data[ pushConstants.uboIndex ].localToClip, float4( positions[ vertexId ], 1 ) );
    
    vsOut.uv.y = 1.0f - vsOut.uv.y;

    return vsOut;
}

float4 mainFS( VSOutput vsOut ) : SV_Target
{
    const float4 tex1 = textures[ pushConstants.texture1Index ].Sample( sLinear, vsOut.uv );
    const float4 tex2 = textures[ pushConstants.texture2Index ].Sample( sLinear, vsOut.uv2 );

    return lerp( tex1, tex2, 0.5f );
}
