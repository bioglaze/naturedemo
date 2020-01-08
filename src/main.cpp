// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/*
  Naturedemo
  
  Testing water and sky rendering etc.

  Author: Timo Wiren
  Modified: 2020-01-08
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
void aeRenderMesh( const aeMesh& mesh, const aeShader& shader, const Matrix& localToClip, const Matrix& localToView, const aeTexture2D& texture, const aeTexture2D& texture2, const Vec3& lightDir, unsigned uboIndex );
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
    Matrix trans;
    trans.SetTranslation( transform.localPosition );
    Matrix::Multiply( trans, transform.localMatrix, transform.localMatrix );
}

static void TransformMoveForward( Transform& transform, float amount )
{
	if( amount != 0 )
	{
		transform.localPosition += transform.localRotation * Vec3( 0, 0, amount );
	}
}

static void TransformMoveRight( Transform& transform, float amount )
{
    if (amount != 0)
    {
        transform.localPosition += transform.localRotation * Vec3( amount, 0, 0 );
    }
}

static void TransformMoveUp( Transform& transform, float amount )
{
    if (amount != 0)
    {
        transform.localPosition += transform.localRotation * Vec3( 0, amount, 0 );
    }
}

static bool IsAlmost( float f1, float f2 )
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
    aeFile wave1nFile = aeLoadFile( "wave1_n.tga" );
    aeFile noiseFile = aeLoadFile( "perlin_noise.tga" );
    aeFile planeFile = aeLoadFile( "plane.a3d" );
    aeFile terrainFile = aeLoadFile( "terrain.a3d" );
    aeFile grassFile = aeLoadFile( "grass.tga" );

    aeShader waterShader = aeCreateShader( waterVertFile, waterFragFile );
    aeShader skyShader = aeCreateShader( skyVertFile, skyFragFile );
    aeShader groundShader = aeCreateShader( groundVertFile, groundFragFile );
    aeTexture2D gliderTex = aeLoadTexture( gliderFile, aeTextureFlags::SRGB | aeTextureFlags::GenerateMips );
    aeTexture2D wave1Tex = aeLoadTexture( wave1File, aeTextureFlags::SRGB | aeTextureFlags::GenerateMips );
    aeTexture2D wave1nTex = aeLoadTexture( wave1nFile, aeTextureFlags::Empty );
    aeTexture2D grassTex = aeLoadTexture( grassFile, aeTextureFlags::SRGB | aeTextureFlags::GenerateMips );
    //aeTexture2D noiseTex = aeLoadTexture( noiseFile, aeTextureFlags::SRGB );
    aeTexture2D sky1Tex = wave1Tex;
    aeTexture2D sky2Tex = wave1Tex;
    aeMesh water = aeLoadMeshFile( planeFile );
    aeMesh sky = aeCreatePlane();
    //aeMesh ground = aeCreatePlane();
    aeMesh ground = aeLoadMeshFile( terrainFile );

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

    bool isMouseDown = false;

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
			else if( event.type == aeWindowEvent::Type::KeyDown && event.keyCode == aeWindowEvent::KeyCode::S)
			{
				TransformMoveForward( cameraTransform, -0.5f );
			}
			else if( event.type == aeWindowEvent::Type::KeyDown && event.keyCode == aeWindowEvent::KeyCode::W)
			{
				TransformMoveForward( cameraTransform, 0.5f );
			}
            else if (event.type == aeWindowEvent::Type::KeyDown && event.keyCode == aeWindowEvent::KeyCode::A)
            {
                TransformMoveRight( cameraTransform, 0.5f );
            }
            else if (event.type == aeWindowEvent::Type::KeyDown && event.keyCode == aeWindowEvent::KeyCode::D)
            {
                TransformMoveRight( cameraTransform, -0.5f );
            }
            else if (event.type == aeWindowEvent::Type::KeyDown && event.keyCode == aeWindowEvent::KeyCode::E)
            {
                TransformMoveUp( cameraTransform, -0.5f );
            }
            else if (event.type == aeWindowEvent::Type::KeyDown && event.keyCode == aeWindowEvent::KeyCode::Q)
            {
                TransformMoveUp( cameraTransform, 0.5f );
            }
            else if (event.type == aeWindowEvent::Type::KeyDown && event.keyCode == aeWindowEvent::KeyCode::Escape)
            {
                shouldQuit = true;
            }

            if (event.type == aeWindowEvent::Type::Mouse1Down)
            {
                isMouseDown = true;
                lastMouseX = event.x;
                lastMouseY = height - event.y;
            }
            else if (event.type == aeWindowEvent::Type::Mouse1Up)
            {
                isMouseDown = false;
                lastMouseX = event.x;
                lastMouseY = height - event.y;
            }

            if (event.type == aeWindowEvent::Type::MouseMove && isMouseDown)
            {
                x = event.x;
                y = height - event.y;
                const float deltaX = float( x - lastMouseX );
                const float deltaY = float( y - lastMouseY );
                lastMouseX = x;
                lastMouseY = y;

                TransformOffsetRotate( cameraTransform, { 0, 1, 0 }, -deltaX / 50.0f );
#if VK_USE_PLATFORM_XCB_KHR
                TransformOffsetRotate( cameraTransform, { 1, 0, 0 }, deltaY / 50.0f );
#else
                TransformOffsetRotate( cameraTransform, { 1, 0, 0 }, -deltaY / 50.0f );
#endif
            }
        }

        TransformSolveLocalMatrix( cameraTransform );

        rotationMatrix.MakeRotationXYZ( 0, 0, 0 );

        waterMatrix.MakeIdentity();
        waterMatrix.Scale( 34, 2, 34 );
        Matrix::Multiply( waterMatrix, rotationMatrix, waterMatrix );
        waterMatrix.Translate( { 20, -4, 15 } );
        Matrix waterView;
        Matrix::Multiply( waterMatrix, cameraTransform.localMatrix, waterView );
        Matrix::Multiply( waterView, viewToClip, waterMatrix );

        skyMatrix.MakeIdentity();
        skyMatrix.Scale( 18, 2, 28 );
        skyMatrix.Translate( { 0, 4, 5 } );
        Matrix skyView;
        Matrix::Multiply( skyMatrix, cameraTransform.localMatrix, skyView );
        Matrix::Multiply( skyView, viewToClip, skyMatrix );

        groundMatrix.MakeIdentity();
        //groundMatrix.Scale( 0.5f, 0.5f, 0.5f );
        Quaternion planeRot;
        planeRot.FromAxisAngle( { 1, 0, 0 }, 90 );
        Matrix rotMat;
        planeRot.GetMatrix( rotMat );
        Matrix::Multiply( groundMatrix, rotMat, groundMatrix );
        groundMatrix.Translate( { 2, -10, 5 } );

        Matrix groundView;
        Matrix::Multiply( groundMatrix, cameraTransform.localMatrix, groundView );
        Matrix::Multiply( groundView, viewToClip, groundMatrix );

        aeBeginFrame();
        aeBeginRenderPass();

        Vec3 lightDir{ 0, 1, 1 };
        lightDir.Normalize();

        aeRenderMesh( water, waterShader, waterMatrix, waterView, wave1Tex, wave1nTex, lightDir, 0 );
        aeRenderMesh( sky, skyShader, skyMatrix, skyView, sky1Tex, sky2Tex, lightDir, 1 );
        aeRenderMesh( ground, groundShader, groundMatrix, groundView, grassTex, grassTex, lightDir, 2 );

        aeEndRenderPass();
        aeEndFrame();
    }

    delete[] waterVertFile.data;
    delete[] waterFragFile.data;
    delete[] skyVertFile.data;
    delete[] skyFragFile.data;
    delete[] groundVertFile.data;
    delete[] groundFragFile.data;
    delete[] gliderFile.data;
    delete[] wave1File.data;
    delete[] planeFile.data;
    delete[] noiseFile.data;

    return 0;
}
