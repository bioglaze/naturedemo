// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "window.hpp"
#include <Windows.h>
#include <xinput.h>
#include <crtdbg.h>

struct aeWindowImpl
{
    aeWindowEvent events[ 15 ];
    unsigned eventCount = 0;
    unsigned windowWidth = 0;
    unsigned windowHeight = 0;
    unsigned windowWidthWithoutTitleBar = 0;
    unsigned windowHeightWithoutTitleBar = 0;
    bool isGamePadConnected = false;
    HWND hwnd = nullptr;
    aeWindowEvent::KeyCode keyMap[ 256 ];
};

aeWindowImpl windows[ 1 ];

static void InitKeyMap()
{
    for (int keyIndex = 0; keyIndex < 256; ++keyIndex)
    {
        windows[ 0 ].keyMap[ keyIndex ] = aeWindowEvent::KeyCode::None;
    }

    windows[ 0 ].keyMap[ 13 ] = aeWindowEvent::KeyCode::Enter;
    windows[ 0 ].keyMap[ 37 ] = aeWindowEvent::KeyCode::Left;
    windows[ 0 ].keyMap[ 38 ] = aeWindowEvent::KeyCode::Up;
    windows[ 0 ].keyMap[ 39 ] = aeWindowEvent::KeyCode::Right;
    windows[ 0 ].keyMap[ 40 ] = aeWindowEvent::KeyCode::Down;
    windows[ 0 ].keyMap[ 27 ] = aeWindowEvent::KeyCode::Escape;
    windows[ 0 ].keyMap[ 32 ] = aeWindowEvent::KeyCode::Space;

    windows[ 0 ].keyMap[ 65 ] = aeWindowEvent::KeyCode::A;
    windows[ 0 ].keyMap[ 66 ] = aeWindowEvent::KeyCode::B;
    windows[ 0 ].keyMap[ 67 ] = aeWindowEvent::KeyCode::C;
    windows[ 0 ].keyMap[ 68 ] = aeWindowEvent::KeyCode::D;
    windows[ 0 ].keyMap[ 69 ] = aeWindowEvent::KeyCode::E;
    windows[ 0 ].keyMap[ 70 ] = aeWindowEvent::KeyCode::F;
    windows[ 0 ].keyMap[ 71 ] = aeWindowEvent::KeyCode::G;
    windows[ 0 ].keyMap[ 72 ] = aeWindowEvent::KeyCode::H;
    windows[ 0 ].keyMap[ 73 ] = aeWindowEvent::KeyCode::I;
    windows[ 0 ].keyMap[ 74 ] = aeWindowEvent::KeyCode::J;
    windows[ 0 ].keyMap[ 75 ] = aeWindowEvent::KeyCode::K;
    windows[ 0 ].keyMap[ 76 ] = aeWindowEvent::KeyCode::L;
    windows[ 0 ].keyMap[ 77 ] = aeWindowEvent::KeyCode::M;
    windows[ 0 ].keyMap[ 78 ] = aeWindowEvent::KeyCode::N;
    windows[ 0 ].keyMap[ 79 ] = aeWindowEvent::KeyCode::O;
    windows[ 0 ].keyMap[ 80 ] = aeWindowEvent::KeyCode::P;
    windows[ 0 ].keyMap[ 81 ] = aeWindowEvent::KeyCode::Q;
    windows[ 0 ].keyMap[ 82 ] = aeWindowEvent::KeyCode::R;
    windows[ 0 ].keyMap[ 83 ] = aeWindowEvent::KeyCode::S;
    windows[ 0 ].keyMap[ 84 ] = aeWindowEvent::KeyCode::T;
    windows[ 0 ].keyMap[ 85 ] = aeWindowEvent::KeyCode::U;
    windows[ 0 ].keyMap[ 86 ] = aeWindowEvent::KeyCode::V;
    windows[ 0 ].keyMap[ 87 ] = aeWindowEvent::KeyCode::W;
    windows[ 0 ].keyMap[ 88 ] = aeWindowEvent::KeyCode::X;
    windows[ 0 ].keyMap[ 89 ] = aeWindowEvent::KeyCode::Y;
    windows[ 0 ].keyMap[ 90 ] = aeWindowEvent::KeyCode::Z;
}

typedef DWORD WINAPI x_input_get_state( DWORD dwUserIndex, XINPUT_STATE* pState );

DWORD WINAPI XInputGetStateStub( DWORD /*dwUserIndex*/, XINPUT_STATE* /*pState*/ )
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

static x_input_get_state* XInputGetState_ = XInputGetStateStub;

typedef DWORD WINAPI x_input_set_state( DWORD dwUserIndex, XINPUT_VIBRATION* pVibration );

DWORD WINAPI XInputSetStateStub( DWORD /*dwUserIndex*/, XINPUT_VIBRATION* /*pVibration*/ )
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

static x_input_set_state* XInputSetState_ = XInputSetStateStub;

static void InitGamePad()
{
    HMODULE XInputLibrary = LoadLibraryA( "xinput1_4.dll" );

    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA( "xinput9_1_0.dll" );
    }

    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA( "xinput1_3.dll" );
    }

    if (XInputLibrary)
    {
        XInputGetState_ = (x_input_get_state*)GetProcAddress( XInputLibrary, "XInputGetState" );
        XInputSetState_ = (x_input_set_state*)GetProcAddress( XInputLibrary, "XInputSetState" );

        XINPUT_STATE controllerState;

        windows[ 0 ].isGamePadConnected = XInputGetState_( 0, &controllerState ) == ERROR_SUCCESS;
    }
    else
    {
        //ae3d::System::Print( "Could not init gamepad.\n" );
    }
}

static float ProcessGamePadStickValue( SHORT value, SHORT deadZoneThreshold )
{
    if (value < -deadZoneThreshold)
    {
        return (float)((value + deadZoneThreshold) / (32768.0f - deadZoneThreshold));
    }
    else if (value > deadZoneThreshold)
    {
        return (float)((value - deadZoneThreshold) / (32767.0f - deadZoneThreshold));
    }

    return 0;
}

static void PumpGamePadEvents()
{
    XINPUT_STATE controllerState;

    if (XInputGetState_( 0, &controllerState ) == ERROR_SUCCESS)
    {
        XINPUT_GAMEPAD* pad = &controllerState.Gamepad;

        float avgX = ProcessGamePadStickValue( pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE );
        float avgY = ProcessGamePadStickValue( pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE );

        windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::GamePadLeftThumbState;
        windows[ 0 ].events[ windows[ 0 ].eventCount ].gamePadThumbX = avgX;
        windows[ 0 ].events[ windows[ 0 ].eventCount ].gamePadThumbY = avgY;

        avgX = ProcessGamePadStickValue( pad->sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE );
        avgY = ProcessGamePadStickValue( pad->sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE );

        windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::GamePadRightThumbState;
        windows[ 0 ].events[ windows[ 0 ].eventCount ].gamePadThumbX = avgX;
        windows[ 0 ].events[ windows[ 0 ].eventCount ].gamePadThumbY = avgY;

        if ((pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0)
        {
            windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::GamePadButtonDPadUp;
        }
        if ((pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0)
        {
            windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::GamePadButtonDPadDown;
        }
        if ((pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0)
        {
            windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::GamePadButtonDPadLeft;
        }
        if ((pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0)
        {
            windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::GamePadButtonDPadRight;
        }
        if ((pad->wButtons & XINPUT_GAMEPAD_A) != 0)
        {
            windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::GamePadButtonA;
        }
        if ((pad->wButtons & XINPUT_GAMEPAD_B) != 0)
        {
            windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::GamePadButtonB;
        }
        if ((pad->wButtons & XINPUT_GAMEPAD_X) != 0)
        {
            windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::GamePadButtonX;
        }
        if ((pad->wButtons & XINPUT_GAMEPAD_Y) != 0)
        {
            windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::GamePadButtonY;
        }
        if ((pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0)
        {
            windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::GamePadButtonLeftShoulder;
        }
        if ((pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0)
        {
            windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::GamePadButtonRightShoulder;

        }
        if ((pad->wButtons & XINPUT_GAMEPAD_START) != 0)
        {
            windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::GamePadButtonStart;
        }
        if ((pad->wButtons & XINPUT_GAMEPAD_BACK) != 0)
        {
            windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::GamePadButtonBack;
        }
    }
}

static LRESULT CALLBACK WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch (message)
    {
    case WM_SYSKEYUP:
    case WM_KEYUP:
        windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::KeyUp;
        windows[ 0 ].events[ windows[ 0 ].eventCount - 1 ].keyCode = windows[ 0 ].keyMap[ (unsigned)wParam ];
    break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::KeyDown;
        windows[ 0 ].events[ windows[ 0 ].eventCount - 1 ].keyCode = windows[ 0 ].keyMap[ (unsigned)wParam ];
    }
    break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    {
        windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = message == WM_LBUTTONDOWN ? aeWindowEvent::Type::Mouse1Down : aeWindowEvent::Type::Mouse1Up;
        windows[ 0 ].events[ windows[ 0 ].eventCount ].x = LOWORD( lParam );
        windows[ 0 ].events[ windows[ 0 ].eventCount ].y = windows[ 0 ].windowHeightWithoutTitleBar - HIWORD( lParam );
    }
    break;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    {
        windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = message == WM_RBUTTONDOWN ? aeWindowEvent::Type::Mouse2Down : aeWindowEvent::Type::Mouse2Up;
        windows[ 0 ].events[ windows[ 0 ].eventCount ].x = LOWORD( lParam );
        windows[ 0 ].events[ windows[ 0 ].eventCount ].y = windows[ 0 ].windowHeightWithoutTitleBar - HIWORD( lParam );
    }
    break;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = message == WM_MBUTTONDOWN ? aeWindowEvent::Type::MouseMiddleDown : aeWindowEvent::Type::MouseMiddleUp;
        windows[ 0 ].events[ windows[ 0 ].eventCount ].x = LOWORD( lParam );
        windows[ 0 ].events[ windows[ 0 ].eventCount ].y = windows[ 0 ].windowHeightWithoutTitleBar - HIWORD( lParam );
    break;
	case WM_MOUSEWHEEL:
	{
		int delta = GET_WHEEL_DELTA_WPARAM( wParam );
        windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = delta < 0 ? aeWindowEvent::Type::MouseWheelScrollDown : aeWindowEvent::Type::MouseWheelScrollUp;
        windows[ 0 ].events[ windows[ 0 ].eventCount ].x = LOWORD( lParam );
        windows[ 0 ].events[ windows[ 0 ].eventCount ].y = windows[ 0 ].windowHeightWithoutTitleBar - HIWORD( lParam );
	}
	break;
	case WM_MOUSEMOVE:
        windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::MouseMove;
        windows[ 0 ].events[ windows[ 0 ].eventCount ].x = LOWORD( lParam );
        windows[ 0 ].events[ windows[ 0 ].eventCount ].y = windows[ 0 ].windowHeightWithoutTitleBar - HIWORD( lParam );
    break;
    case WM_CLOSE:
        windows[ 0 ].events[ windows[ 0 ].eventCount++ ].type = aeWindowEvent::Type::Close;
        windows[ 0 ].events[ 1 ].type = aeWindowEvent::Type::Close;
    break;
    }

    return DefWindowProc( hWnd, message, wParam, lParam );
}

aeWindow aeCreateWindow( unsigned width, unsigned height, const char* title )
{
#if _DEBUG
    _CrtSetDbgFlag( _CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF );
    _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
    _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );
#endif

    aeWindow outWindow;

    windows[ outWindow.index ].windowWidth = width == 0 ? GetSystemMetrics( SM_CXSCREEN ) : width;
    windows[ outWindow.index ].windowHeight = height == 0 ? GetSystemMetrics( SM_CYSCREEN ) : height;

    const HINSTANCE hInstance = GetModuleHandle( nullptr );
    const bool fullscreen = (width == 0 && height == 0);

    WNDCLASSEX wc = {};

    wc.cbSize = sizeof( WNDCLASSEX );
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = "WindowClass1";
    wc.hIcon = static_cast<HICON>( LoadImage( nullptr, "glider.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE ) );

    RegisterClassEx( &wc );

    const int xPos = (GetSystemMetrics( SM_CXSCREEN ) - windows[ outWindow.index ].windowWidth) / 2;
    const int yPos = (GetSystemMetrics( SM_CYSCREEN ) - windows[ outWindow.index ].windowHeight) / 2;

    windows[ outWindow.index ].hwnd = CreateWindowExA( fullscreen ? WS_EX_TOOLWINDOW | WS_EX_TOPMOST : 0,
        "WindowClass1", "Window",
        fullscreen ? WS_POPUP : (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU),
        xPos, yPos,
        windows[ outWindow.index ].windowWidth, windows[ outWindow.index ].windowHeight,
        nullptr, nullptr, hInstance, nullptr );
    
    outWindow.connection = (xcb_connection_t*)windows[ outWindow.index ].hwnd;

    SetWindowTextA( windows[ outWindow.index ].hwnd, title );
    ShowWindow( windows[ outWindow.index ].hwnd, SW_SHOW );

    RECT rect = {};
    GetClientRect( windows[ outWindow.index ].hwnd, &rect );
    windows[ outWindow.index ].windowWidthWithoutTitleBar = rect.right;
    windows[ outWindow.index ].windowHeightWithoutTitleBar = rect.bottom;
    
    InitGamePad();
    InitKeyMap();

    return outWindow;
}

void aeGetRenderArea( const aeWindow& window, unsigned& outWidth, unsigned& outHeight )
{
    outWidth = windows[ window.index ].windowWidthWithoutTitleBar;
    outHeight = windows[ window.index ].windowHeightWithoutTitleBar;
}

void aePumpWindowEvents( const aeWindow& window )
{
    MSG msg;

    while (PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ))
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    if (windows[ window.index ].isGamePadConnected)
    {
        PumpGamePadEvents();
    }
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

void aeDestroyWindow( aeWindow /*window*/ )
{

}
