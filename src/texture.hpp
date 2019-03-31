#pragma once

struct aeTexture2D { int index = -1; };
enum aeTextureFlags { Empty = 0, GenerateMips = 1, SRGB = 2 };

aeTexture2D aeLoadTexture( const struct aeFile& file, unsigned flags );
