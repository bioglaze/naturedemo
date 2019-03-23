#include <stdio.h>
#include "file.hpp"
#include "shader.hpp"
#include "window.hpp"

int main()
{
    constexpr unsigned width = 1920;
    constexpr unsigned height = 1080;
    
    aeWindow window = aeCreateWindow( width, height, "Nature Demo" );
    aeFile waterVertFile = aeLoadFile( "water_vert.spv" );
    aeFile waterFragFile = aeLoadFile( "water_frag.spv" );
    aeShader waterShader = aeCreateShader( waterVertFile, waterFragFile );
    
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
    }
    
    return 0;
}
