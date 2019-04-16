#pragma once

struct alignas( 16 ) Matrix
{
    static void Invert( const Matrix& matrix, Matrix& out );
    static void InverseTranspose( const float m[ 16 ], float* out );
    static void Multiply( const Matrix& ma, const Matrix& mb, Matrix& out );

    Matrix() noexcept { MakeIdentity(); }
    void InitFrom( const float* mat );
    void MakeIdentity();
    void MakeProjection( float fovDegrees, float aspect, float nearDepth, float farDepth );
    void MakeProjection( float left, float right, float bottom, float top, float nearDepth, float farDepth );
    void MakeLookAt( const struct Vec3& eye, const Vec3& center, const Vec3& up );
    void Scale( float x, float y, float z );
    void SetTranslation( const Vec3& translation );
    void TransformPoint( const float vec[ 4 ], const Matrix& mat, float out[ 4 ] );
    void Transpose( Matrix& out ) const;
    void Translate( const Vec3& );

    float m[ 16 ];
};
