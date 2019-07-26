// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/*
  Naturedemo
  
  Testing water and sky rendering etc.

  Author: Timo Wiren
  Modified: 2019-07-26
 */
#include <stdio.h>
#include <math.h>
#include "file.hpp"
#include "matrix.hpp"
#include "mesh.hpp"
#include "quaternion.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "vec3.hpp"
#include "window.hpp"

void aeInitRenderer( unsigned width, unsigned height, struct xcb_connection_t* connection, unsigned window );
void aeRenderMesh( const aeMesh& mesh, const aeShader& shader, const Matrix& localToClip, const aeTexture2D& texture, const aeTexture2D& texture2, const Vec3& lightDir, unsigned uboIndex );
void aeBeginFrame();
void aeEndFrame();
void aeBeginRenderPass();
void aeEndRenderPass();

struct Transform
{
    Matrix localMatrix;
    Quaternion localRotation;
    Vec3 localPosition;
    float localScale = 1;
};

static void TransformSolveLocalMatrix( Transform& transform )
{
    transform.localRotation.GetMatrix( transform.localMatrix );
    transform.localMatrix.Scale( transform.localScale, transform.localScale, transform.localScale );
    transform.localMatrix.SetTranslation( transform.localPosition );
}

static void TransformMoveForward( Transform& transform, float amount )
{
	if( amount != 0 )
	{
		transform.localPosition += transform.localRotation * Vec3( 0, 0, amount );
	}
}

static float IsAlmost( float f1, float f2 )
{
    return fabsf( f1 - f2 ) < 0.0001f;
}

void TransformOffsetRotate( Transform& transform, const Vec3& axis, float angleDeg )
{
    Quaternion rot;
    rot.FromAxisAngle( axis, angleDeg );
    
    Quaternion newRotation;

    if (IsAlmost( axis.y, 0 ))
    {
        newRotation = transform.localRotation * rot;
    }
    else
    {
        newRotation = rot * transform.localRotation;
    }

    newRotation.Normalize();
    
    if ((IsAlmost( axis.x, 1 ) || IsAlmost( axis.x, -1 )) && IsAlmost( axis.y, 0 ) && IsAlmost( axis.z, 0 ) &&
        newRotation.FindTwist( Vec3( 1.0f, 0.0f, 0.0f ) ) > 0.9999f)
    {
        return;
    }

    transform.localRotation = newRotation;
}

void TransformLookAt( Transform& transform, const Vec3& localPosition, const Vec3& center, const Vec3& up )
{
    Matrix lookAt;
    lookAt.MakeLookAt( transform.localPosition, center, up );
    transform.localRotation.FromMatrix( lookAt );
    transform.localPosition = localPosition;
}

int main()
{
    unsigned width = 1920;
    unsigned height = 1080;
    
    aeWindow window = aeCreateWindow( width, height, "Nature Demo" );
    aeGetRenderArea( window, width, height );
    aeInitRenderer( width, height, window.connection, window.window );
    
    aeFile waterVertFile = aeLoadFile( "water_vs.spv" );
    aeFile waterFragFile = aeLoadFile( "water_fs.spv" );
    aeFile skyVertFile = aeLoadFile( "sky_vs.spv" );
    aeFile skyFragFile = aeLoadFile( "sky_fs.spv" );
    aeFile groundVertFile = aeLoadFile( "ground_vs.spv" );
    aeFile groundFragFile = aeLoadFile( "ground_fs.spv" );
    aeFile gliderFile = aeLoadFile( "glider.tga" );
    aeFile wave1File = aeLoadFile( "wave1.tga" );
    
    aeShader waterShader = aeCreateShader( waterVertFile, waterFragFile );
    aeShader skyShader = aeCreateShader( skyVertFile, skyFragFile );
    aeShader groundShader = aeCreateShader( groundVertFile, groundFragFile );
    aeTexture2D gliderTex = aeLoadTexture( gliderFile, aeTextureFlags::SRGB );
    aeTexture2D wave1Tex = aeLoadTexture( wave1File, aeTextureFlags::SRGB );
    aeTexture2D sky1Tex = wave1Tex;
    aeTexture2D sky2Tex = wave1Tex;
    aeMesh water = aeCreatePlane();
    aeMesh sky = aeCreatePlane();
    aeMesh ground = aeCreatePlane();

    if (window.index == -1)
    {
        return 0;
    }

    bool shouldQuit = false;
    int x = 0;
    int y = 0;
    int lastMouseX = 0;
    int lastMouseY = 0;

    Transform cameraTransform;
    Matrix viewToClip;
    viewToClip.MakeProjection( 45.0f, width / float( height ), 0.1f, 200.0f );
    
    TransformLookAt( cameraTransform, { 0, 0, 0 }, { 0, 0, -400 }, { 0, 1, 0 } );

    Matrix waterMatrix;
    Matrix rotationMatrix;

    Matrix skyMatrix;

    Matrix groundMatrix;

    while (!shouldQuit)
    {
        aePumpWindowEvents( window );

        bool eventsHandled = false;

        while (!eventsHandled)
        {
            const aeWindowEvent& event = aePopWindowEvent( window );

            if (event.type == aeWindowEvent::Type::Empty)
            {
                eventsHandled = true;
            }
			else if( event.type == aeWindowEvent::Type::KeyDown && event.keyCode == aeWindowEvent::KeyCode::W)
			{
				TransformMoveForward( cameraTransform, 0.5f );
			}
			else if( event.type == aeWindowEvent::Type::KeyDown && event.keyCode == aeWindowEvent::KeyCode::S )
			{
				TransformMoveForward( cameraTransform, -0.5f );
			}
			else if (event.type == aeWindowEvent::Type::KeyDown && event.keyCode == aeWindowEvent::KeyCode::Escape)
            {
                shouldQuit = true;
            }

            if (event.type == aeWindowEvent::Type::MouseMove)
            {
                x = event.x;
                y = height - event.y;
                const float deltaX = float( x - lastMouseX );
                const float deltaY = float( y - lastMouseY );
                lastMouseX = x;
                lastMouseY = y;

                TransformOffsetRotate( cameraTransform, { 0, 1, 0 }, -deltaX / 50.0f );
                TransformOffsetRotate( cameraTransform, { 1, 0, 0 }, -deltaY / 50.0f );
            }
        }

        TransformSolveLocalMatrix( cameraTransform );

        rotationMatrix.MakeRotationXYZ( 0, 0, 0 );

        waterMatrix.MakeIdentity();
        waterMatrix.Scale( 2, 2, 2 );
        Matrix::Multiply( waterMatrix, rotationMatrix, waterMatrix );
        waterMatrix.Translate( { 0, -2, 5 } );
        Matrix::Multiply( waterMatrix, cameraTransform.localMatrix, waterMatrix );
        Matrix::Multiply( waterMatrix, viewToClip, waterMatrix );

        skyMatrix.MakeIdentity();
        skyMatrix.Scale( 2, 2, 2 );
        skyMatrix.Translate( { 0, 2, 5 } );
        Matrix::Multiply( skyMatrix, cameraTransform.localMatrix, skyMatrix );
        Matrix::Multiply( skyMatrix, viewToClip, skyMatrix );

        groundMatrix.MakeIdentity();
        groundMatrix.Translate( { 2, -1, 5 } );
        Matrix::Multiply( groundMatrix, cameraTransform.localMatrix, groundMatrix );
        Matrix::Multiply( groundMatrix, viewToClip, groundMatrix );

        aeBeginFrame();
        aeBeginRenderPass();

        const Vec3 lightDir{ 0, 1, 0 };

        aeRenderMesh( water, waterShader, waterMatrix, gliderTex, wave1Tex, lightDir, 0 );
        aeRenderMesh( sky, skyShader, skyMatrix, sky1Tex, sky2Tex, lightDir, 1 );
        aeRenderMesh( ground, groundShader, groundMatrix, gliderTex, gliderTex, lightDir, 2 );

        aeEndRenderPass();
        aeEndFrame();
    }
    
    return 0;
}
