#pragma once

struct aeTexture2D { int index = -1; };
struct aeTextureCube { int index = -1; };
enum aeTextureFlags { Empty = 0, GenerateMips = 1, SRGB = 2 };

aeTexture2D aeLoadTexture2D( const struct aeFile& file, unsigned flags );
aeTextureCube aeLoadTextureCube( const aeFile& negX, const aeFile& posX, const aeFile& negY, const aeFile& posY, const aeFile& negZ, const aeFile& posZ, unsigned flags );

