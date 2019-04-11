#include <stdio.h>
#include "file.hpp"
#include "mesh.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "window.hpp"

void aeInitRenderer( unsigned width, unsigned height, struct xcb_connection_t* connection, unsigned window );
void aeRenderMesh( const aeMesh& mesh );
void aeBeginFrame();
void aeEndFrame();
void aeBeginRenderPass();
void aeEndRenderPass();

int main()
{
    unsigned width = 1920;
    unsigned height = 1080;
    
    aeWindow window = aeCreateWindow( width, height, "Nature Demo" );
    aeGetRenderArea( window, width, height );
    aeInitRenderer( width, height, window.connection, window.window );
    
    aeFile waterVertFile = aeLoadFile( "water_vs.spv" );
    aeFile waterFragFile = aeLoadFile( "water_fs.spv" );
    aeFile gliderFile = aeLoadFile( "glider.tga" );
    aeShader waterShader = aeCreateShader( waterVertFile, waterFragFile );
    aeTexture2D gliderTex = aeLoadTexture( gliderFile, aeTextureFlags::SRGB );
    aeMesh plane = aeCreatePlane();
    
    if (window.index == -1)
    {
        return 0;
    }

    bool shouldQuit = false;

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
            else if (event.type == aeWindowEvent::Type::KeyDown)
            {
                shouldQuit = true;
            }
        }

        aeBeginFrame();
        aeBeginRenderPass();

        aeRenderMesh( plane );

        aeEndRenderPass();
        aeEndFrame();
    }
    
    return 0;
}
