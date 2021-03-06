// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "mesh.hpp"
#include "file.hpp"
#include "vertexbuffer.hpp"

struct MeshImpl
{
    VertexBuffer positions;
    VertexBuffer normals;
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

aeMesh aeLoadMeshFile( const struct aeFile& a3dFile )
{    
    aeMesh outMesh;
    outMesh.index = meshCount++;

    unsigned char* pointer = &a3dFile.data[ 8 ];
    pointer += 4;

    //meshes[ outMesh.index ].aabbMin.x = (float)(*pointer);
    pointer += 4;
    //meshes[ outMesh.index ].subMeshes[ m ].aabbMin.y = (float)(*pointer);
    pointer += 4;
    //meshes[ outMesh.index ].subMeshes[ m ].aabbMin.z = (float)(*pointer);

    pointer += 4;
    //meshes[ outMesh.index ].subMeshes[ m ].aabbMax.x = (float)(*pointer);
    pointer += 4;
    //meshes[ outMesh.index ].subMeshes[ m ].aabbMax.y = (float)(*pointer);
    pointer += 4;
    //meshes[ outMesh.index ].subMeshes[ m ].aabbMax.z = (float)(*pointer);

    pointer += 4;
    const unsigned faceCount = *((unsigned*)pointer);
    pointer += 4;
    meshes[ outMesh.index ].indices = CreateVertexBuffer( pointer, faceCount * 2 * 3, BufferType::Ushort, BufferUsage::Index, "indices" );
    pointer += faceCount * 2 * 3;

    if (faceCount % 2 != 0)
    {
        pointer += 2;
    }

    const unsigned vertexCount = *((unsigned*)pointer);
    pointer += 4;
    meshes[ outMesh.index ].positions = CreateVertexBuffer( pointer, vertexCount * 3 * 4, BufferType::Float3, BufferUsage::Vertex, "positions" );
    pointer += vertexCount * 3 * 4;
    meshes[ outMesh.index ].uvs = CreateVertexBuffer( pointer, vertexCount * 2 * 4, BufferType::Float2, BufferUsage::Vertex, "uvs" );
    pointer += vertexCount * 2 * 4;
    meshes[ outMesh.index ].normals = CreateVertexBuffer( pointer, vertexCount * 3 * 4, BufferType::Float3, BufferUsage::Vertex, "normals" );
    //pointer += vertexCount * 3 * 4;
    
    return outMesh;
}

const struct VertexBuffer& GetIndices( const aeMesh& mesh )
{
    return meshes[ mesh.index ].indices;
}

const struct VertexBuffer& GetPositions( const aeMesh& mesh )
{
    return meshes[ mesh.index ].positions;
}

const struct VertexBuffer& GetUVs( const aeMesh& mesh )
{
    return meshes[ mesh.index ].uvs;
}
