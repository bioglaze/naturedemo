#pragma once

enum class BlendMode { Off, AlphaBlend, Additive };
enum class CullMode { Off, Front, Back };
enum class DepthMode { NoneWriteOff, LessOrEqualWriteOn, LessOrEqualWriteOff };
enum class Topology { Triangles, Lines };
enum class FillMode { Solid, Wireframe };

enum class BufferUsage { Vertex, Index };
enum class BufferType { Uint, Ushort, Float2, Float3, Float4 };

struct VertexBuffer
{
    int index = -1;
    unsigned count = 0;
};

VertexBuffer CreateVertexBuffer( const void* data, unsigned dataBytes, BufferType bufferType, BufferUsage bufferUsage, const char* debugName );
