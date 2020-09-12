#include "shader_common.h"

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
