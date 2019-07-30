#pragma once

struct aeMesh
{
    int index = -1;
};

aeMesh aeCreatePlane();
const struct VertexBuffer& GetIndices( const aeMesh& mesh );
aeMesh aeLoadMeshFile( const struct aeFile& a3dFile );
