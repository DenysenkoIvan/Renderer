#include "Input.h"

#include <cstring>

#include <Framework/Common.h>

bool Input::s_keyIsDown[(int)Key::KeysCount] = { 0 };
bool Input::s_keyIsPressed[(int)Key::KeysCount] = { 0 };
bool Input::s_keyIsReleased[(int)Key::KeysCount] = { 0 };
std::wstring Input::s_charsTyped;
bool Input::s_isOutOfScreen = false;;
bool Input::s_isMouseRaw = false;
MousePos Input::s_mousePos{};
MousePos Input::s_mousePosDelta{};
MousePos Input::s_mousePosRawDelta{};
float Input::s_mouseWheelDelta{ 0.0f };

bool MousePos::IsOutOfScreen() const
{
    return x == -1 && y == -1;
}

MousePos MousePos::GetOutOfScreen()
{
    return MousePos(-1, -1);
}

void Input::NewFrame()
{
    std::memset(s_keyIsPressed, 0, sizeof(s_keyIsPressed));
    std::memset(s_keyIsReleased, 0, sizeof(s_keyIsReleased));
    s_mousePosDelta.x = 0;
    s_mousePosDelta.y = 0;
    s_mousePosRawDelta.x = 0;
    s_mousePosRawDelta.y = 0;
    s_mouseWheelDelta = 0.0f;
    s_charsTyped.clear();
}

bool Input::IsKeyPressed(Key key)
{
    ValidateKey(key);
    return s_keyIsPressed[(int)key];
}
bool Input::IsKeyReleased(Key key)
{
    ValidateKey(key);
    return s_keyIsReleased[(int)key];
}

bool Input::IsKeyDown(Key key)
{
    ValidateKey(key);
    return s_keyIsDown[(int)key];
}

const std::wstring& Input::GetCharsTyped()
{
    return s_charsTyped;
}

bool Input::IsMouseRaw()
{
    return s_isMouseRaw;
}

bool Input::IsMouseOutOfScreen()
{
    return s_isOutOfScreen;
}

const MousePos& Input::GetMousePos()
{
    return s_mousePos;
}

const MousePos& Input::GetMousePosDelta()
{
    return s_mousePosDelta;
}

const MousePos& Input::GetMousePosRawDelta()
{
    return s_mousePosRawDelta;
}

float Input::GetMouseWheelDelta()
{
    return s_mouseWheelDelta;
}

void Input::SetMouseOutOfScreen(bool isOutOfScreen)
{
    s_isOutOfScreen = isOutOfScreen;
}

void Input::SetMouseRaw(bool isRaw)
{
    if (!isRaw)
    {
        s_mousePosRawDelta.x = 0;
        s_mousePosRawDelta.y = 0;
    }

    s_isMouseRaw = isRaw;
}

void Input::SetKeyPressed(Key key)
{
    ValidateKey(key);
    int keyIndex = (int)key;
    s_keyIsPressed[keyIndex] = true;
    s_keyIsReleased[keyIndex] = false;
    s_keyIsDown[keyIndex] = true;
}

void Input::SetKeyReleased(Key key)
{
    ValidateKey(key);
    int keyIndex = (int)key;
    s_keyIsPressed[keyIndex] = false;
    s_keyIsReleased[keyIndex] = true;
    s_keyIsDown[keyIndex] = false;
}

void Input::AddCharTyped(wchar_t c)
{
    s_charsTyped += c;
}

void Input::SetMousePos(MousePos pos)
{
    s_mousePosDelta.x = pos.x - s_mousePos.x;
    s_mousePosDelta.y = pos.y - s_mousePos.y;
    s_mousePos = pos;
}

void Input::UpdateMousePosRawDelta(MousePos delta)
{
    s_mousePosRawDelta.x += delta.x;
    s_mousePosRawDelta.y += delta.y;
}

void Input::UpdateMouseWheelDelta(float delta)
{
    s_mouseWheelDelta += delta;
}

void Input::ValidateKey(Key key)
{
    Assert(key > Key::None && key < Key::KeysCount);
}