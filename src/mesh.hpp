#pragma once

struct aeMesh
{
    int index = -1;
};

aeMesh aeCreatePlane();
const struct VertexBuffer& GetIndices( const aeMesh& mesh );
const struct VertexBuffer& GetPositions( const aeMesh& mesh );
const struct VertexBuffer& GetUVs( const aeMesh& mesh );
aeMesh aeLoadMeshFile( const struct aeFile& a3dFile );
