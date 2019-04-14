#include "mesh.hpp"
#include "vertexbuffer.hpp"

struct MeshImpl
{
    VertexBuffer positions;
    VertexBuffer uvs;
    VertexBuffer indices;
};

MeshImpl meshes[ 20 ];
unsigned meshCount = 0;

aeMesh aeCreatePlane()
{
    aeMesh outMesh;
    outMesh.index = meshCount++;

    constexpr float positions[ 30 * 3 ] =
    {
        1, -1, -1,  1, -1, 1, -1, -1, 1, 1, 1, -1, -1, 1, -1, -1, 1, 1,
        1, -1, -1, 1, 1, -1, 1, 1, 1, 1, -1, 1, 1, 1, 1, -1, -1, 1,
        -1, -1, 1, -1, 1, 1, -1, -1, -1, 1, 1, -1, 1, -1, -1, -1, -1, -1,
        -1, -1, -1, 1, -1, -1, 1, 1, 1, 1, 1, -1, 1, -1, 1, 1, -1, -1,
        1, 1, 1, -1, 1, 1, -1, 1, 1, -1, 1, -1, -1, 1, -1, 1, 1, -1
    };

    constexpr float uvs[ 30 * 2 ] =
    {
        0, 1, 0, 2, -1, 2, 0, 0, -1, 0, -1, 1, 0, 1, 0, 0, -1, 0, 0, 1,
        0, 0, -1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 2, -1, 2, -1, 1, 0, 1,
        0, 1, 0, 0, -1, 1, 0, 1, 0, 0, -1, 0, 0, 0, 1, 0, -1, 1, 0, 1
    };

    constexpr unsigned short indices[ 12 * 3 ] =
    {
        0, 1, 2, 18, 19, 2, 3, 4, 5, 20, 21, 5, 6, 7, 8, 22, 23, 8,
        9, 10, 11, 24, 25, 11, 12, 13, 14, 26, 27, 14, 15, 16, 17, 28, 29, 17
    };
    
    meshes[ outMesh.index ].positions = CreateVertexBuffer( positions, sizeof( positions ), BufferType::Float3, BufferUsage::Vertex, "positions" );
    meshes[ outMesh.index ].uvs = CreateVertexBuffer( uvs, sizeof( uvs ), BufferType::Float2, BufferUsage::Vertex, "uvs" );
    meshes[ outMesh.index ].indices = CreateVertexBuffer( indices, sizeof( indices ), BufferType::Ushort, BufferUsage::Index, "indices" );

    return outMesh;
}

const struct VertexBuffer& GetIndices( const aeMesh& mesh )
{
    return meshes[ mesh.index ].indices;
}