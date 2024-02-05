#pragma once

#include <string>

enum class Key : int
{
    None,

    // Mouse buttons
    LButton, RButton, MButton,

    // Keyboard buttons
    Esc, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Tilde, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0, MinusSign, EqualSign, Backspace,
    Tab, Capslock, LShift, LCtrl, LWin, LAlt, Space, RAlt, RWin, RCtrl, RShift, Enter,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LBracket, RBracket, BackSlash, Semicolon, Apostrophe, Comma, Period, ForwardSlash,
    Delete,
    LeftArrow, UpArrow, RightArrow, DownArrow,

    KeysCount
};

struct MousePos
{
    int x = -1;
    int y = -1;

    MousePos() = default;
    MousePos(int x, int y)
        : x(x), y(y)
    {}

    bool IsOutOfScreen() const;

    static MousePos GetOutOfScreen();
};

class Input
{
public:
    static void NewFrame();

    static bool IsKeyPressed(Key key);
    static bool IsKeyReleased(Key key);
    static bool IsKeyDown(Key key);
    static const std::wstring& GetCharsTyped();
    static bool IsMouseRaw();
    static bool IsMouseOutOfScreen();
    static const MousePos& GetMousePos();
    static const MousePos& GetMousePosDelta();
    static const MousePos& GetMousePosRawDelta();
    static float GetMouseWheelDelta();

    static void SetMouseOutOfScreen(bool isOutOfScreen);
    static void SetMouseRaw(bool isRaw);
    static void SetKeyPressed(Key key);
    static void SetKeyReleased(Key key);
    static void AddCharTyped(wchar_t c);
    static void SetMousePos(MousePos pos);
    static void UpdateMousePosRawDelta(MousePos delta);
    static void UpdateMouseWheelDelta(float delta);

private:
    static void ValidateKey(Key key);

private:
    static bool s_keyIsDown[(int)Key::KeysCount];
    static bool s_keyIsPressed[(int)Key::KeysCount];
    static bool s_keyIsReleased[(int)Key::KeysCount];
    static std::wstring s_charsTyped;
    static bool s_isOutOfScreen;
    static bool s_isMouseRaw;
    static MousePos s_mousePos;
    static MousePos s_mousePosDelta;
    static MousePos s_mousePosRawDelta;
    static float s_mouseWheelDelta;
};