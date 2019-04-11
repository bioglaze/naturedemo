#pragma once

struct aeWindow
{
    struct xcb_connection_t* connection = nullptr;
    int index = 0;
    unsigned window = 0;
    unsigned width = 0;
    unsigned height = 0;
};

struct aeWindowEvent
{
    enum class Type
    {
        Empty, Close, Mouse1Down, Mouse1Up, Mouse2Down, Mouse2Up, MouseMiddleDown, MouseMiddleUp,
        MouseWheelScrollDown, MouseWheelScrollUp, MouseMove, KeyDown, KeyUp,
        GamePadButtonA, GamePadButtonB, GamePadButtonX,
        GamePadButtonY, GamePadButtonDPadUp, GamePadButtonDPadDown,
        GamePadButtonDPadLeft, GamePadButtonDPadRight, GamePadButtonStart,
        GamePadButtonBack, GamePadButtonLeftShoulder, GamePadButtonRightShoulder,
        GamePadLeftThumbState, GamePadRightThumbState
    };
    
    enum class KeyCode
    {
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        Left, Right, Up, Down, Space, Escape, Enter, None
    };

    Type type = Type::Empty;
    KeyCode keyCode = KeyCode::None;
    int x = 0;
    int y = 0;
    float gamePadThumbX = 0;
    /// Gamepad's thumb y in range [-1, 1]. Event type indicates left or right thumb.
    float gamePadThumbY = 0;
};
    
aeWindow aeCreateWindow( unsigned width, unsigned height, const char* title );
void aeGetRenderArea( const aeWindow& window, unsigned& outWidth, unsigned& outHeight );
void aePumpWindowEvents( const aeWindow& window );
const aeWindowEvent& aePopWindowEvent( const aeWindow& window );
void aeDestroyWindow( aeWindow window );
