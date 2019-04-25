#include "window.hpp"
#include <dirent.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include <unistd.h>
#include <stdio.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>

struct aeWindowImpl
{
    aeWindowEvent events[ 15 ];
    xcb_key_symbols_t* keySymbols = nullptr;
    int eventCount = 0;
};

struct GamePad
{
    bool isActive = false;
    int fd;
    int buttonA;
    int buttonB;
    int buttonX;
    int buttonY;
    int buttonBack;
    int buttonStart;
    int buttonLeftShoulder;
    int buttonRightShoulder;
    int dpadXaxis;
    int dpadYaxis;
    int leftThumbX;
    int leftThumbY;
    int rightThumbX;
    int rightThumbY;
    short deadZone;
    float lastLeftThumbX;
    float lastLeftThumbY;
    float lastRightThumbX;
    float lastRightThumbY;
};

GamePad gamePad;
aeWindowImpl windows[ 10 ];

float ProcessGamePadStickValue( short value, short deadZoneThreshold )
{
    float result = 0;
        
    if (value < -deadZoneThreshold)
    {
        result = (value + deadZoneThreshold) / (32768.0f - deadZoneThreshold);
    }
    else if (value > deadZoneThreshold)
    {
        result = (value - deadZoneThreshold) / (32767.0f - deadZoneThreshold);
    }

    return result;
}

void InitGamePad()
{
    DIR* dir = opendir( "/dev/input" );
    dirent* result = readdir( dir );

    while (result != nullptr)
    {
        dirent& entry = *result;
        
        if ((entry.d_name[0] == 'j') && (entry.d_name[1] == 's'))
        {
            char full_device_path[ 267 ];
            snprintf( full_device_path, sizeof( full_device_path ), "%s/%s", "/dev/input", entry.d_name );
            int fd = open( full_device_path, O_RDONLY );

            if (fd < 0)
            {
                // Permissions could cause this code path.
                continue;
            }

            char name[ 128 ];
            ioctl( fd, JSIOCGNAME( 128 ), name);

            int version;
            ioctl( fd, JSIOCGVERSION, &version );
            uint8_t axes;
            ioctl( fd, JSIOCGAXES, &axes );
            uint8_t buttons;
            ioctl( fd, JSIOCGBUTTONS, &buttons );
            gamePad.fd = fd;
            gamePad.isActive = true;
            // XBox One Controller values. Should also work for 360 Controller.
            gamePad.buttonA = 0;
            gamePad.buttonB = 1;
            gamePad.buttonX = 2;
            gamePad.buttonY = 3;
            gamePad.buttonStart = 7;
            gamePad.buttonBack = 6;
            gamePad.buttonLeftShoulder = 4;
            gamePad.buttonRightShoulder = 5;
            gamePad.dpadXaxis = 6;
            gamePad.dpadYaxis = 7;
            gamePad.leftThumbX = 0;
            gamePad.leftThumbY = 1;
            gamePad.rightThumbX = 3;
            gamePad.rightThumbY = 4;
            gamePad.deadZone = 7849;
            
            fcntl( fd, F_SETFL, O_NONBLOCK );
        }

        result = readdir( dir );
    }

    closedir( dir );
}

void aeGetRenderArea( const aeWindow& window, unsigned& outWidth, unsigned& outHeight )
{
    outWidth = window.width;
    outHeight = window.height;
}

aeWindow aeCreateWindow( unsigned width, unsigned height, const char* title )
{
    aeWindow outWindow;
    outWindow.index = 0;

    outWindow.connection = xcb_connect( nullptr, nullptr );

    if (xcb_connection_has_error( outWindow.connection ))
    {
        outWindow.index = -1;
        return outWindow;
    }

    xcb_screen_t* s = xcb_setup_roots_iterator( xcb_get_setup( outWindow.connection ) ).data;
    outWindow.window = xcb_generate_id( outWindow.connection );
    outWindow.width = width == 0 ? s->width_in_pixels : width;
    outWindow.height = height == 0 ? s->height_in_pixels : height;
    windows[ outWindow.index ].keySymbols = xcb_key_symbols_alloc( outWindow.connection );
    
    const unsigned mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    const unsigned eventMask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
                               XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION;
    const unsigned values[ 2 ] { s->white_pixel, eventMask };
    
    xcb_create_window( outWindow.connection, s->root_depth, outWindow.window, s->root,
                       10, 10,
                       outWindow.width,
                       outWindow.height,
                       1,
                       XCB_WINDOW_CLASS_INPUT_OUTPUT, s->root_visual,
                       mask, values );

    xcb_map_window( outWindow.connection, outWindow.window );
    xcb_flush( outWindow.connection );

    xcb_size_hints_t hints;
    xcb_icccm_size_hints_set_min_size( &hints, outWindow.width, outWindow.height );
    xcb_icccm_size_hints_set_max_size( &hints, outWindow.width, outWindow.height );
    xcb_icccm_set_wm_size_hints( outWindow.connection, outWindow.window, XCB_ATOM_WM_NORMAL_HINTS, &hints );

    xcb_change_property( outWindow.connection, XCB_PROP_MODE_REPLACE, outWindow.window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, 10, title );

    if (width == 0 && height == 0)
    {
        xcb_ewmh_connection_t EWMH;
        xcb_intern_atom_cookie_t* EWMHCookie = xcb_ewmh_init_atoms( outWindow.connection, &EWMH );

        if (!xcb_ewmh_init_atoms_replies( &EWMH, EWMHCookie, nullptr ))
        {
            outWindow.index = -1;
            return outWindow;
        }

        xcb_change_property( outWindow.connection, XCB_PROP_MODE_REPLACE, outWindow.window, EWMH._NET_WM_STATE, XCB_ATOM_ATOM, 32, 1, &(EWMH._NET_WM_STATE_FULLSCREEN) );
        
        xcb_generic_error_t* error;
        xcb_get_window_attributes_reply_t* reply = xcb_get_window_attributes_reply( outWindow.connection,
                                                                                    xcb_get_window_attributes( outWindow.connection,
                                                                                                               outWindow.window ), &error );

        if (!reply)
        {
            outWindow.index = -1;
            return outWindow;
        }

        free( reply );
    }

    InitGamePad();
    return outWindow;
}

void aeDestroyWindow( aeWindow window )
{
    xcb_destroy_window( window.connection, window.window ); 
    xcb_disconnect( window.connection );
}

const aeWindowEvent& aePopWindowEvent( const aeWindow& window )
{
    aeWindowImpl& win = windows[ window.index ];

    if (win.eventCount == 0)
    {
        win.events[ 0 ].type = aeWindowEvent::Type::Empty;
        return win.events[ 0 ];
    }
    
    return win.events[ win.eventCount-- ];
}

void aePumpWindowEvents( const aeWindow& window )
{
    xcb_generic_event_t* event;
    
    while ((event = xcb_poll_for_event( window.connection )))
    {
        if (windows[ window.index ].eventCount >= 14)
        {
            free( event );
            return;
        }
        
        const uint8_t responseType = event->response_type & ~0x80;

        if (responseType == XCB_BUTTON_PRESS || responseType == XCB_BUTTON_RELEASE)
        {
            xcb_button_press_event_t* bp = (xcb_button_press_event_t *)event;
            printf("button press: %d\n", bp->detail);
            if (bp->detail == 2)
            {
                windows[ window.index ].events[ windows[ window.index ].eventCount++ ].type = responseType == XCB_BUTTON_RELEASE ? aeWindowEvent::Type::MouseMiddleUp : aeWindowEvent::Type::MouseMiddleDown;
            }
            else if (bp->detail == 3)
            {
                windows[ window.index ].events[ windows[ window.index ].eventCount++ ].type = responseType == XCB_BUTTON_RELEASE ? aeWindowEvent::Type::Mouse2Up : aeWindowEvent::Type::Mouse2Down;
            }
            else
            {
                windows[ window.index ].events[ windows[ window.index ].eventCount++ ].type = responseType == XCB_BUTTON_RELEASE ? aeWindowEvent::Type::Mouse1Up : aeWindowEvent::Type::Mouse1Down;
            }
            
            windows[ window.index ].events[ windows[ window.index ].eventCount ].x = bp->event_x;
            windows[ window.index ].events[ windows[ window.index ].eventCount ].y = bp->event_y;
        }
        else if (responseType == XCB_KEY_PRESS)
        {
            xcb_key_press_event_t* kp = (xcb_key_press_event_t *)event;
            printf("key code: %d, modifiers: %d\n", kp->event, kp->state);

            windows[ window.index ].events[ windows[ window.index ].eventCount++ ].type = aeWindowEvent::Type::KeyDown;
        }
        else if (responseType == XCB_KEY_RELEASE)
        {
            //xcb_key_press_event_t* kp = (xcb_key_press_event_t *)event;
            windows[ window.index ].events[ windows[ window.index ].eventCount++ ].type = aeWindowEvent::Type::KeyUp;
        }
        else if (responseType == XCB_MOTION_NOTIFY)
        {
            xcb_motion_notify_event_t* motion = (xcb_motion_notify_event_t *)event;
            windows[ window.index ].events[ windows[ window.index ].eventCount++ ].type = aeWindowEvent::Type::MouseMove;
            windows[ window.index ].events[ windows[ window.index ].eventCount ].x = motion->event_x;
            windows[ window.index ].events[ windows[ window.index ].eventCount ].y = motion->event_y;
        }
        
        free( event );
    }

    aeWindowImpl& win = windows[ window.index ];

    if (!gamePad.isActive)
    {
        return;
    }
    
    js_event j;

    while (read( gamePad.fd, &j, sizeof( js_event ) ) == sizeof( js_event ))
    {
        j.type &= ~JS_EVENT_INIT;
            
        if (j.type == JS_EVENT_BUTTON)
        {
            if (j.number == gamePad.buttonA && j.value > 0)
            {
                win.events[ win.eventCount++ ].type = aeWindowEvent::Type::GamePadButtonA;
            }
            else if (j.number == gamePad.buttonB && j.value > 0)
            {
                win.events[ win.eventCount++ ].type = aeWindowEvent::Type::GamePadButtonB;
            }
            else if (j.number == gamePad.buttonX && j.value > 0)
            {
                win.events[ win.eventCount++ ].type = aeWindowEvent::Type::GamePadButtonX;
            }
            else if (j.number == gamePad.buttonY && j.value > 0)
            {
                win.events[ win.eventCount++ ].type = aeWindowEvent::Type::GamePadButtonY;
            }
            else if (j.number == gamePad.buttonStart && j.value > 0)
            {
                win.events[ win.eventCount++ ].type = aeWindowEvent::Type::GamePadButtonStart;
            }
            else if (j.number == gamePad.buttonBack && j.value > 0)
            {
                win.events[ win.eventCount++ ].type = aeWindowEvent::Type::GamePadButtonBack;
            }
        }
        else if (j.type == JS_EVENT_AXIS)
        {
            if (j.number == gamePad.leftThumbX)
            {
                const float x = ProcessGamePadStickValue( j.value, gamePad.deadZone );
                win.events[ win.eventCount ].type = aeWindowEvent::Type::GamePadLeftThumbState;
                win.events[ win.eventCount ].gamePadThumbX = x;
                win.events[ win.eventCount ].gamePadThumbY = gamePad.lastLeftThumbY;
                gamePad.lastLeftThumbX = x;
                ++win.eventCount;
            }
            else if (j.number == gamePad.leftThumbY)
            {
                const float y = ProcessGamePadStickValue( j.value, gamePad.deadZone );
                win.events[ win.eventCount ].type = aeWindowEvent::Type::GamePadLeftThumbState;
                win.events[ win.eventCount ].gamePadThumbX = gamePad.lastLeftThumbX;
                win.events[ win.eventCount ].gamePadThumbY = -y;
                gamePad.lastLeftThumbY = -y;
                ++win.eventCount;
            }
            else if (j.number == gamePad.rightThumbX)
            {
                const float x = ProcessGamePadStickValue( j.value, gamePad.deadZone );
                win.events[ win.eventCount ].type = aeWindowEvent::Type::GamePadRightThumbState;
                win.events[ win.eventCount ].gamePadThumbX = x;
                win.events[ win.eventCount ].gamePadThumbY = gamePad.lastRightThumbY;
                gamePad.lastRightThumbX = x;
                ++win.eventCount;
            }
            else if (j.number == gamePad.rightThumbY)
            {
                const float y = ProcessGamePadStickValue( j.value, gamePad.deadZone );
                win.events[ win.eventCount ].type = aeWindowEvent::Type::GamePadRightThumbState;
                win.events[ win.eventCount ].gamePadThumbX = gamePad.lastRightThumbX;
                win.events[ win.eventCount ].gamePadThumbY = -y;
                gamePad.lastRightThumbY = -y;
                ++win.eventCount;
            }
            else if (j.number == gamePad.dpadXaxis)
            {
                if (j.value > 0)
                {
                    win.events[ win.eventCount++ ].type = aeWindowEvent::Type::GamePadButtonDPadRight;
                }
                if (j.value < 0)
                {
                    win.events[ win.eventCount++ ].type = aeWindowEvent::Type::GamePadButtonDPadLeft;
                }
            }
            else if (j.number == gamePad.dpadYaxis)
            {
                if (j.value < 0)
                {
                    win.events[ win.eventCount++ ].type = aeWindowEvent::Type::GamePadButtonDPadUp;
                }
                if (j.value > 0)
                {
                    win.events[ win.eventCount++ ].type = aeWindowEvent::Type::GamePadButtonDPadDown;
                }
            }
        }
    }
}

