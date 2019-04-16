#include "matrix.hpp"
#include <math.h>
#include "quaternion.hpp"
#include "vec3.hpp"

#include <pmmintrin.h>

void Matrix::Multiply( const Matrix& ma, const Matrix& mb, Matrix& out )
{
    Matrix result;
    Matrix matA = ma;
    Matrix matB = mb;

    const float* a = matA.m;
    const float* b = matB.m;
    __m128 a_line, b_line, r_line;

    for (int i = 0; i < 16; i += 4)
    {
        // unroll the first step of the loop to avoid having to initialize r_line to zero
        a_line = _mm_load_ps( b );
        b_line = _mm_set1_ps( a[ i ] );
        r_line = _mm_mul_ps( a_line, b_line );

        for (int j = 1; j < 4; j++)
        {
            a_line = _mm_load_ps( &b[ j * 4 ] );
            b_line = _mm_set1_ps(  a[ i + j ] );
            r_line = _mm_add_ps(_mm_mul_ps( a_line, b_line ), r_line);
        }

        _mm_store_ps( &result.m[ i ], r_line );
    }

    out = result;
}

void Matrix::TransformPoint( const float vec[ 4 ], const Matrix& mat, float out[ 4 ] )
{
    Matrix transpose;
    mat.Transpose( transpose );
    alignas( 16 ) float v4[ 4 ] = { vec[ 0 ], vec[ 1 ], vec[ 2 ], vec[ 3 ] };

    __m128 vec4 = _mm_load_ps( v4 );
    __m128 row1 = _mm_load_ps( &transpose.m[  0 ] );
    __m128 row2 = _mm_load_ps( &transpose.m[  4 ] );
    __m128 row3 = _mm_load_ps( &transpose.m[  8 ] );
    __m128 row4 = _mm_load_ps( &transpose.m[ 12 ] );

    __m128 r1 = _mm_mul_ps( row1, vec4 );
    __m128 r2 = _mm_mul_ps( row2, vec4 );
    __m128 r3 = _mm_mul_ps( row3, vec4 );
    __m128 r4 = _mm_mul_ps( row4, vec4 );

    __m128 sum_01 = _mm_hadd_ps( r1, r2 );
    __m128 sum_23 = _mm_hadd_ps( r3, r4 );
    __m128 result = _mm_hadd_ps( sum_01, sum_23 );
    _mm_store_ps( out, result );
}

void Matrix::InitFrom( const float* mat )
{
    for (int i = 0; i < 16; ++i)
    {
        m[ i ] = mat[ i ];
    }
}

void Matrix::InverseTranspose( const float m[ 16 ], float* out )
{
    float tmp[ 12 ];

    // Calculates pairs for first 8 elements (cofactors)
    tmp[ 0 ] = m[ 10 ] * m[ 15 ];
    tmp[ 1 ] = m[ 11 ] * m[ 14 ];
    tmp[ 2 ] = m[ 9 ] * m[ 15 ];
    tmp[ 3 ] = m[ 11 ] * m[ 13 ];
    tmp[ 4 ] = m[ 9 ] * m[ 14 ];
    tmp[ 5 ] = m[ 10 ] * m[ 13 ];
    tmp[ 6 ] = m[ 8 ] * m[ 15 ];
    tmp[ 7 ] = m[ 11 ] * m[ 12 ];
    tmp[ 8 ] = m[ 8 ] * m[ 14 ];
    tmp[ 9 ] = m[ 10 ] * m[ 12 ];
    tmp[ 10 ] = m[ 8 ] * m[ 13 ];
    tmp[ 11 ] = m[ 9 ] * m[ 12 ];

    // Calculates first 8 elements (cofactors)
    out[ 0 ] = tmp[ 0 ] * m[ 5 ] + tmp[ 3 ] * m[ 6 ] + tmp[ 4 ] * m[ 7 ] - tmp[ 1 ] * m[ 5 ] - tmp[ 2 ] * m[ 6 ] - tmp[ 5 ] * m[ 7 ];
    out[ 1 ] = tmp[ 1 ] * m[ 4 ] + tmp[ 6 ] * m[ 6 ] + tmp[ 9 ] * m[ 7 ] - tmp[ 0 ] * m[ 4 ] - tmp[ 7 ] * m[ 6 ] - tmp[ 8 ] * m[ 7 ];
    out[ 2 ] = tmp[ 2 ] * m[ 4 ] + tmp[ 7 ] * m[ 5 ] + tmp[ 10 ] * m[ 7 ] - tmp[ 3 ] * m[ 4 ] - tmp[ 6 ] * m[ 5 ] - tmp[ 11 ] * m[ 7 ];
    out[ 3 ] = tmp[ 5 ] * m[ 4 ] + tmp[ 8 ] * m[ 5 ] + tmp[ 11 ] * m[ 6 ] - tmp[ 4 ] * m[ 4 ] - tmp[ 9 ] * m[ 5 ] - tmp[ 10 ] * m[ 6 ];
    out[ 4 ] = tmp[ 1 ] * m[ 1 ] + tmp[ 2 ] * m[ 2 ] + tmp[ 5 ] * m[ 3 ] - tmp[ 0 ] * m[ 1 ] - tmp[ 3 ] * m[ 2 ] - tmp[ 4 ] * m[ 3 ];
    out[ 5 ] = tmp[ 0 ] * m[ 0 ] + tmp[ 7 ] * m[ 2 ] + tmp[ 8 ] * m[ 3 ] - tmp[ 1 ] * m[ 0 ] - tmp[ 6 ] * m[ 2 ] - tmp[ 9 ] * m[ 3 ];
    out[ 6 ] = tmp[ 3 ] * m[ 0 ] + tmp[ 6 ] * m[ 1 ] + tmp[ 11 ] * m[ 3 ] - tmp[ 2 ] * m[ 0 ] - tmp[ 7 ] * m[ 1 ] - tmp[ 10 ] * m[ 3 ];
    out[ 7 ] = tmp[ 4 ] * m[ 0 ] + tmp[ 9 ] * m[ 1 ] + tmp[ 10 ] * m[ 2 ] - tmp[ 5 ] * m[ 0 ] - tmp[ 8 ] * m[ 1 ] - tmp[ 11 ] * m[ 2 ];

    // Calculates pairs for second 8 elements (cofactors)
    tmp[ 0 ] = m[ 2 ] * m[ 7 ];
    tmp[ 1 ] = m[ 3 ] * m[ 6 ];
    tmp[ 2 ] = m[ 1 ] * m[ 7 ];
    tmp[ 3 ] = m[ 3 ] * m[ 5 ];
    tmp[ 4 ] = m[ 1 ] * m[ 6 ];
    tmp[ 5 ] = m[ 2 ] * m[ 5 ];
    tmp[ 6 ] = m[ 0 ] * m[ 7 ];
    tmp[ 7 ] = m[ 3 ] * m[ 4 ];
    tmp[ 8 ] = m[ 0 ] * m[ 6 ];
    tmp[ 9 ] = m[ 2 ] * m[ 4 ];
    tmp[ 10 ] = m[ 0 ] * m[ 5 ];
    tmp[ 11 ] = m[ 1 ] * m[ 4 ];

    // Calculates second 8 elements (cofactors)
    out[ 8 ] = tmp[ 0 ] * m[ 13 ] + tmp[ 3 ] * m[ 14 ] + tmp[ 4 ] * m[ 15 ]
        - tmp[ 1 ] * m[ 13 ] - tmp[ 2 ] * m[ 14 ] - tmp[ 5 ] * m[ 15 ];

    out[ 9 ] = tmp[ 1 ] * m[ 12 ] + tmp[ 6 ] * m[ 14 ] + tmp[ 9 ] * m[ 15 ]
        - tmp[ 0 ] * m[ 12 ] - tmp[ 7 ] * m[ 14 ] - tmp[ 8 ] * m[ 15 ];

    out[ 10 ] = tmp[ 2 ] * m[ 12 ] + tmp[ 7 ] * m[ 13 ] + tmp[ 10 ] * m[ 15 ]
        - tmp[ 3 ] * m[ 12 ] - tmp[ 6 ] * m[ 13 ] - tmp[ 11 ] * m[ 15 ];

    out[ 11 ] = tmp[ 5 ] * m[ 12 ] + tmp[ 8 ] * m[ 13 ] + tmp[ 11 ] * m[ 14 ]
        - tmp[ 4 ] * m[ 12 ] - tmp[ 9 ] * m[ 13 ] - tmp[ 10 ] * m[ 14 ];

    out[ 12 ] = tmp[ 2 ] * m[ 10 ] + tmp[ 5 ] * m[ 11 ] + tmp[ 1 ] * m[ 9 ]
        - tmp[ 4 ] * m[ 11 ] - tmp[ 0 ] * m[ 9 ] - tmp[ 3 ] * m[ 10 ];

    out[ 13 ] = tmp[ 8 ] * m[ 11 ] + tmp[ 0 ] * m[ 8 ] + tmp[ 7 ] * m[ 10 ]
        - tmp[ 6 ] * m[ 10 ] - tmp[ 9 ] * m[ 11 ] - tmp[ 1 ] * m[ 8 ];

    out[ 14 ] = tmp[ 6 ] * m[ 9 ] + tmp[ 11 ] * m[ 11 ] + tmp[ 3 ] * m[ 8 ]
        - tmp[ 10 ] * m[ 11 ] - tmp[ 2 ] * m[ 8 ] - tmp[ 7 ] * m[ 9 ];

    out[ 15 ] = tmp[ 10 ] * m[ 10 ] + tmp[ 4 ] * m[ 8 ] + tmp[ 9 ] * m[ 9 ]
        - tmp[ 8 ] * m[ 9 ] - tmp[ 11 ] * m[ 10 ] - tmp[ 5 ] * m[ 8 ];

    // Calculates the determinant.
    const float det = m[ 0 ] * out[ 0 ] + m[ 1 ] * out[ 1 ] + m[ 2 ] * out[ 2 ] + m[ 3 ] * out[ 3 ];
    const float acceptableDelta = 0.0001f;

    if (fabsf( det ) < acceptableDelta)
    {
        for (int i = 0; i < 15; ++i)
        {
            out[ i ] = 0;
        }
        
        out[ 0 ] = out[ 5 ] = out[ 10 ] = out[ 15 ] = 1;

        return;
    }

    for (int i = 0; i < 16; ++i)
    {
        out[ i ] /= det;
    }
}

void Matrix::Invert( const Matrix& matrix, Matrix& out )
{
    float invTrans[ 16 ];
    InverseTranspose( &matrix.m[ 0 ], &invTrans[ 0 ] );
    Matrix iTrans;
    iTrans.InitFrom( &invTrans[ 0 ] );
    iTrans.Transpose( out );
}

void Matrix::MakeLookAt( const Vec3& eye, const Vec3& center, const Vec3& up )
{
    const Vec3 zAxis = (center - eye).Normalized();
    const Vec3 xAxis = Vec3::Cross( up, zAxis ).Normalized();
    const Vec3 yAxis = Vec3::Cross( zAxis, xAxis ).Normalized();

    m[ 0 ] = xAxis.x; m[ 1 ] = xAxis.y; m[ 2 ] = xAxis.z; m[ 3 ] = -Vec3::Dot( xAxis, eye );
    m[ 4 ] = yAxis.x; m[ 5 ] = yAxis.y; m[ 6 ] = yAxis.z; m[ 7 ] = -Vec3::Dot( yAxis, eye );
    m[ 8 ] = zAxis.x; m[ 9 ] = zAxis.y; m[ 10 ] = zAxis.z; m[ 11 ] = -Vec3::Dot( zAxis, eye );
    m[ 12 ] = 0; m[ 13 ] = 0; m[ 14 ] = 0; m[ 15 ] = 1;
}

void Matrix::MakeIdentity()
{
    for (int i = 0; i < 15; ++i)
    {
        m[ i ] = 0;
    }

    m[ 0 ] = m[ 5 ] = m[ 10 ] = m[ 15 ] = 1;
}

void Matrix::MakeProjection( float fovDegrees, float aspect, float nearDepth, float farDepth )
{
    const float f = 1.0f / tanf( (0.5f * fovDegrees) * 3.14159265358979f / 180.0f );

    const float proj[] =
    {
        f / aspect, 0, 0, 0,
        0, -f, 0, 0,
        0, 0, farDepth / (nearDepth - farDepth), -1,
        0, 0, (nearDepth * farDepth) / (nearDepth - farDepth), 0
    };

    InitFrom( &proj[ 0 ] );
}

void Matrix::MakeProjection( float left, float right, float bottom, float top, float nearDepth, float farDepth )
{
    const float tx = -((right + left) / (right - left));
    const float ty = -((bottom + top) / (bottom - top));
    const float tz = nearDepth / (nearDepth - farDepth);

    const float proj[] =
    {
            2.0f / (right - left), 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f / (bottom - top), 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f / (nearDepth - farDepth), 0.0f,
            tx, ty, tz, 1.0f
    };

    InitFrom( &proj[ 0 ] );
}

void Matrix::Scale( float x, float y, float z )
{
    Matrix scale;
    scale.MakeIdentity();
    
    scale.m[  0 ] = x;
    scale.m[  5 ] = y;
    scale.m[ 10 ] = z;
    
    Multiply( *this, scale, *this );
}

void Matrix::Transpose( Matrix& out ) const
{
    float tmp[ 16 ];

    tmp[  0 ] = m[  0 ];
    tmp[  1 ] = m[  4 ];
    tmp[  2 ] = m[  8 ];
    tmp[  3 ] = m[ 12 ];
    tmp[  4 ] = m[  1 ];
    tmp[  5 ] = m[  5 ];
    tmp[  6 ] = m[  9 ];
    tmp[  7 ] = m[ 13 ];
    tmp[  8 ] = m[  2 ];
    tmp[  9 ] = m[  6 ];
    tmp[ 10 ] = m[ 10 ];
    tmp[ 11 ] = m[ 14 ];
    tmp[ 12 ] = m[  3 ];
    tmp[ 13 ] = m[  7 ];
    tmp[ 14 ] = m[ 11 ];
    tmp[ 15 ] = m[ 15 ];

    out.InitFrom( &tmp[ 0 ] );
}

void Matrix::SetTranslation( const Vec3& translation )
{
    m[ 12 ] = translation.x;
    m[ 13 ] = translation.y;
    m[ 14 ] = translation.z;
}

void Matrix::Translate( const Vec3& v )
{
    Matrix translateMatrix;
    translateMatrix.MakeIdentity();
    
    translateMatrix.m[ 12 ] = v.x;
    translateMatrix.m[ 13 ] = v.y;
    translateMatrix.m[ 14 ] = v.z;
    
    Multiply( *this, translateMatrix, *this );
}

void Vec3::Normalize()
{
    const float invLen = 1.0f / sqrtf( x * x + y * y + z * z );
    x *= invLen;
    y *= invLen;
    z *= invLen;
}

Vec3 Vec3::Normalized() const
{
    Vec3 res = *this;
    res.Normalize();
    return res;
}

Quaternion::Quaternion( const struct Vec3& v, float aw )
{
    x = v.x;
    y = v.y;
    z = v.z;
    w = aw;
}

Vec3 Quaternion::operator*( const Vec3& vec ) const
{
    const Vec3 wComponent( w, w, w );
    const Vec3 two( 2.0f, 2.0f, 2.0f );
    
    const Vec3 vT = two * Vec3::Cross( Vec3( x, y, z ), vec );
    return vec + (wComponent * vT) + Vec3::Cross( Vec3( x, y, z ), vT );
}

Quaternion Quaternion::operator*( const Quaternion& aQ ) const
{
    return Quaternion( Vec3( w * aQ.x + x * aQ.w + y * aQ.z - z * aQ.y,
                             w * aQ.y + y * aQ.w + z * aQ.x - x * aQ.z,
                             w * aQ.z + z * aQ.w + x * aQ.y - y * aQ.x ),
                             w * aQ.w - x * aQ.x - y * aQ.y - z * aQ.z );
}

void Quaternion::FromAxisAngle( const Vec3& axis, float angleDeg )
{
    const float angleRad = angleDeg * (3.1418693659f / 360.0f);    
    const float sinAngle = sinf( angleRad );
    
    x = axis.x * sinAngle;
    y = axis.y * sinAngle;
    z = axis.z * sinAngle;
    w = cosf( angleRad );
}

void Quaternion::Normalize()
{
    const float mag2 = w * w + x * x + y * y + z * z;
    const float acceptableDelta = 0.00001f;
    
    if (fabsf( mag2 ) > acceptableDelta && fabsf( mag2 - 1.0f ) > acceptableDelta)
    {
        const float oneOverMag = 1.0f / sqrtf( mag2 );
        
        x *= oneOverMag;
        y *= oneOverMag;
        z *= oneOverMag;
        w *= oneOverMag;
    }
}

void Quaternion::FromMatrix( const Matrix& mat )
{
    float t;
    
    if (mat.m[ 10 ] < 0)
    {
        if (mat.m[ 0 ] > mat.m[ 5 ])
        {
            t = 1 + mat.m[ 0 ] - mat.m[ 5 ] - mat.m[ 10 ];
            *this = Quaternion( Vec3( t, mat.m[ 1 ] + mat.m[ 4 ], mat.m[ 8 ] + mat.m[ 2 ] ), mat.m[ 6 ] - mat.m[ 9 ] );
        }
        else
        {
            t = 1 - mat.m[ 0 ] + mat.m[ 5 ] - mat.m[ 10 ];
            *this = Quaternion( Vec3( mat.m[ 1 ] + mat.m[ 4 ], t, mat.m[ 6 ] + mat.m[ 9 ] ), mat.m[ 8 ] - mat.m[ 2 ] );
        }
    }
    else
    {
        if (mat.m[ 0 ] < -mat.m[ 5 ])
        {
            t = 1 - mat.m[ 0 ] - mat.m[ 5 ] + mat.m[ 10 ];
            *this = Quaternion( Vec3( mat.m[ 8 ] + mat.m[ 2 ], mat.m[ 6 ] + mat.m[ 9 ], t ), mat.m[ 1 ] - mat.m[ 4 ] );
        }
        else
        {
            t = 1 + mat.m[ 0 ] + mat.m[ 5 ] + mat.m[ 10 ];
            *this = Quaternion( Vec3( mat.m[ 6 ] - mat.m[ 9 ], mat.m[ 8 ] - mat.m[ 2 ], mat.m[ 1 ] - mat.m[ 4 ] ), t );
        }
    }
    
    const float factor = 0.5f / sqrtf( t );
    x *= factor;
    y *= factor;
    z *= factor;
    w *= factor;
}

void Quaternion::GetMatrix( Matrix& outMatrix ) const
{
    const float x2 = x * x;
    const float y2 = y * y;
    const float z2 = z * z;
    const float xy = x * y;
    const float xz = x * z;
    const float yz = y * z;
    const float wx = w * x;
    const float wy = w * y;
    const float wz = w * z;
    
    outMatrix.m[ 0] = 1 - 2 * (y2 + z2);
    outMatrix.m[ 1] = 2 * (xy - wz);
    outMatrix.m[ 2] = 2 * (xz + wy);
    outMatrix.m[ 3] = 0;
    outMatrix.m[ 4] = 2 * (xy + wz);
    outMatrix.m[ 5] = 1 - 2 * (x2 + z2);
    outMatrix.m[ 6] = 2 * (yz - wx);
    outMatrix.m[ 7] = 0;
    outMatrix.m[ 8] = 2 * (xz - wy);
    outMatrix.m[ 9] = 2 * (yz + wx);
    outMatrix.m[10] = 1 - 2 * (x2 + y2);
    outMatrix.m[11] = 0;
    outMatrix.m[12] = 0;
    outMatrix.m[13] = 0;
    outMatrix.m[14] = 0;
    outMatrix.m[15] = 1;
}
