/*
  Naturedemo
  
  Testing water and sky rendering etc.

  Author: Timo Wiren
  Modified: 2019-07-02
 */
#include <stdio.h>
#include "file.hpp"
#include "matrix.hpp"
#include "mesh.hpp"
#include "quaternion.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "vec3.hpp"
#include "window.hpp"

void aeInitRenderer( unsigned width, unsigned height, struct xcb_connection_t* connection, unsigned window );
void aeRenderMesh( const aeMesh& mesh, const aeShader& shader, const Matrix& localToClip, const aeTexture2D& texture, unsigned uboIndex );
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

void TransformSolveLocalMatrix( Transform& transform )
{
    transform.localRotation.GetMatrix( transform.localMatrix );
    transform.localMatrix.Scale( transform.localScale, transform.localScale, transform.localScale );
    transform.localMatrix.SetTranslation( transform.localPosition );
}

void TransformMoveForward( Transform& transform, float amount )
{
	if( amount != 0 )
	{
		transform.localPosition += transform.localRotation * Vec3( 0, 0, amount );
	}
}

void TransformOffsetRotate( Transform& transform, const Vec3& axis, float angleDeg )
{
    Quaternion rot;
    rot.FromAxisAngle( axis, angleDeg );
    
    Quaternion newRotation = rot * transform.localRotation;
    newRotation.Normalize();
    
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
    
    aeShader waterShader = aeCreateShader( waterVertFile, waterFragFile );
    aeShader skyShader = aeCreateShader( skyVertFile, skyFragFile );
    aeShader groundShader = aeCreateShader( groundVertFile, groundFragFile );
    aeTexture2D gliderTex = aeLoadTexture( gliderFile, aeTextureFlags::SRGB );
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
    float deltaX = 0;
    float deltaY = 0;
    int lastMouseX = 0;
    int lastMouseY = 0;

    Transform cameraTransform;
    Matrix viewToClip;
    viewToClip.MakeProjection( 45.0f, width / float( height ), 0.1f, 200.0f );
    
    //cameraTransform.localMatrix.MakeLookAt( { 0, 0, 0 }, { 0, 0, -400 }, { 0, 1, 0 } );
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
				TransformMoveForward( cameraTransform, 0.1f );
			}
			else if( event.type == aeWindowEvent::Type::KeyDown && event.keyCode == aeWindowEvent::KeyCode::S )
			{
				TransformMoveForward( cameraTransform, -0.1f );
			}
			else if (event.type == aeWindowEvent::Type::KeyDown && event.keyCode == aeWindowEvent::KeyCode::Escape)
            {
                shouldQuit = true;
            }

            if (event.type == aeWindowEvent::Type::MouseMove)
            {
                x = event.x;
                y = height - event.y;
                deltaX = float( x - lastMouseX );
                deltaY = float( y - lastMouseY );
                lastMouseX = x;
                lastMouseY = y;
                TransformOffsetRotate( cameraTransform, { 0, 1, 0 }, -x / 200.0f );
                //TransformOffsetRotate( cameraTransform, { 1, 0, 0 }, y / 200.0f );
            }
        }

        TransformSolveLocalMatrix( cameraTransform );

        rotationMatrix.MakeRotationXYZ( 45, 0, 0 );
        waterMatrix.MakeIdentity();
        Matrix::Multiply( waterMatrix, rotationMatrix, waterMatrix );
        waterMatrix.Translate( { 0, 0, 5 } );
        Matrix::Multiply( waterMatrix, cameraTransform.localMatrix, waterMatrix );
        Matrix::Multiply( waterMatrix, viewToClip, waterMatrix );

        skyMatrix.MakeIdentity();
        skyMatrix.Translate( { 1, 0, 5 } );
        Matrix::Multiply( skyMatrix, cameraTransform.localMatrix, skyMatrix );
        Matrix::Multiply( skyMatrix, viewToClip, skyMatrix );

        groundMatrix.MakeIdentity();
        groundMatrix.Translate( { 1, 1, 5 } );
        Matrix::Multiply( groundMatrix, cameraTransform.localMatrix, groundMatrix );
        Matrix::Multiply( groundMatrix, viewToClip, groundMatrix );

        aeBeginFrame();
        aeBeginRenderPass();

        aeRenderMesh( water, waterShader, waterMatrix, gliderTex, 0 );
        aeRenderMesh( sky, skyShader, skyMatrix, gliderTex, 1 );
        aeRenderMesh( ground, groundShader, groundMatrix, gliderTex, 2 );

        aeEndRenderPass();
        aeEndFrame();
    }
    
    return 0;
}
