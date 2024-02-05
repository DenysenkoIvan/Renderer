#include "Window.h"

#include <algorithm>

#include <hidusage.h>

#include <Framework/Common.h>

static std::array<Key, 128> s_keyboardScancodesToKeys
{
    Key::None,
    Key::Esc,
    Key::Num1,
    Key::Num2,
    Key::Num3,
    Key::Num4,
    Key::Num5,
    Key::Num6,
    Key::Num7,
    Key::Num8,
    Key::Num9,
    Key::Num0,
    Key::MinusSign,
    Key::EqualSign,
    Key::Backspace,
    Key::Tab,
    Key::Q,
    Key::W,
    Key::E,
    Key::R,
    Key::T,
    Key::Y,
    Key::U,
    Key::I,
    Key::O,
    Key::P,
    Key::LBracket,
    Key::RBracket,
    Key::Enter,
    Key::LCtrl,
    Key::A,
    Key::S,
    Key::D,
    Key::F,
    Key::G,
    Key::H,
    Key::J,
    Key::K,
    Key::L,
    Key::Semicolon,
    Key::Apostrophe,
    Key::Tilde,
    Key::LShift,
    Key::None,
    Key::Z,
    Key::X,
    Key::C,
    Key::V,
    Key::B,
    Key::N,
    Key::M,
    Key::Comma,
    Key::Period,
    Key::ForwardSlash,
    Key::RShift,
    Key::None,
    Key::LAlt,
    Key::Space,
    Key::Capslock,
    Key::F1,
    Key::F2,
    Key::F3,
    Key::F4,
    Key::F5,
    Key::F6,
    Key::F7,
    Key::F8,
    Key::F9,
    Key::F10,
    Key::None,
    Key::None,
    Key::None,
    Key::UpArrow,
    Key::None,
    Key::None,
    Key::LeftArrow,
    Key::None,
    Key::RightArrow,
    Key::None,
    Key::None,
    Key::DownArrow,
    Key::None,
    Key::None,
    Key::Delete,
    Key::None,
    Key::None,
    Key::None,
    Key::F11,
    Key::F12,
    Key::None,
    Key::None,
    Key::LWin,
    Key::RWin,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None,
    Key::None
};

Window* Window::s_window = nullptr;

Window::Window(std::wstring_view windowName, uint32_t width, uint32_t height)
{
    WNDCLASS description{};
    description.style = CS_HREDRAW | CS_VREDRAW;
    description.lpfnWndProc = &Window::HandleEvents;
    description.cbClsExtra = 0;
    description.cbWndExtra = sizeof(Window*);
    description.hInstance = GetModuleHandle(0);
    description.hIcon = 0;
    description.hCursor = LoadCursor(NULL, IDC_ARROW);
    description.hbrBackground = 0;
    description.lpszClassName = L"Window class name";

    ATOM result = RegisterClass(&description);

    if (result)
    {
        CreateWindowEx(
            0,
            description.lpszClassName,
            windowName.data(),
            WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT,
            width, height,
            0, 0,
            description.hInstance,
            this
        );

        Assert(m_hwnd, "Window creation failed");

        ResizeNewlyCreatedWindow(width, height);

        RegisterRawKeyboardDevice();
        
        PollEvents();
    }
}

Window::~Window()
{
    if (!m_hwnd)
    {
        Error(m_hwnd, "Window is not created to be destroyed");
        return;
    }

    ReleaseCapture();

    ShowCursor();
    DestroyWindow(m_hwnd);
    m_hwnd = 0;
}

void Window::PollEvents()
{
    ProfileFunction();

    Assert(m_hwnd, "Window not created");

    BeginPollEvents();

    while (true)
    {
        MSG message{};
        BOOL messageResult = PeekMessage(&message, 0, 0, 0, PM_REMOVE);
        if (messageResult)
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        else
        {
            break;
        }
    }

    EndPollEvents();
}

void Window::SetFocused()
{
    SetForegroundWindow(m_hwnd);
    SetFocus(m_hwnd);
}

bool Window::IsInFocus() const
{
    return m_isFocused;
}

void Window::EnterFullscreenBorderless()
{
    if (m_state.isFullscreen)
    {
        LogError("Window is in full borderless already");
        return;
    }

    m_state.prevState.style = GetWindowLong(m_hwnd, GWL_STYLE);
    if (m_state.prevState.style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO mi = { sizeof(mi) };
        GetWindowPlacement(m_hwnd, &m_state.prevState.placement);
        GetMonitorInfo(MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);

        SetWindowLong(m_hwnd, GWL_STYLE, m_state.prevState.style & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(
            m_hwnd, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED
        );

        m_state.isWindowResized = true;
    }

    m_state.isFullscreen = true;
}

void Window::ExitFullscreenBorderless()
{
    if (!m_state.isFullscreen)
    {
        LogError("Window is not in fullscreen borderless");
        return;
    }

    if (m_state.prevState.style & WS_OVERLAPPEDWINDOW)
    {
        SetWindowLong(m_hwnd, GWL_STYLE, m_state.prevState.style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(m_hwnd, &m_state.prevState.placement);
        SetWindowPos(
            m_hwnd, NULL, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED
        );

        m_state.isWindowResized = true;
    }

    m_state.isFullscreen = false;
}

void Window::ShowCursor()
{
    if (!Input::IsMouseRaw())
    {
        LogError("Mouse is not in raw mode");
        return;
    }

    RAWINPUTDEVICE inputDevice{};
    inputDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;
    inputDevice.usUsage = HID_USAGE_GENERIC_MOUSE;
    inputDevice.dwFlags = RIDEV_REMOVE;
    inputDevice.hwndTarget = NULL;
    bool res = RegisterRawInputDevices(&inputDevice, 1, sizeof(inputDevice));

    SetCursorPos(m_prevMousePosBeforeRaw.x, m_prevMousePosBeforeRaw.y);
    
    ::ShowCursor(true);

    Input::SetMouseRaw(false);
}

void Window::HideCursor()
{
    if (Input::IsMouseRaw())
    {
        LogError("Mouse is already in raw mode");
        return;
    }

    RAWINPUTDEVICE inputDevices[2]{};

    RAWINPUTDEVICE mouseDevice{};
    mouseDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;
    mouseDevice.usUsage = HID_USAGE_GENERIC_MOUSE;
    mouseDevice.dwFlags = RIDEV_NOLEGACY | RIDEV_CAPTUREMOUSE;
    mouseDevice.hwndTarget = m_hwnd;

    if (!RegisterRawInputDevices(&mouseDevice, 1, sizeof(mouseDevice)))
    {
        LogError("Failed to register raw mouse device");
        return;
    }

    GetCursorPos(&m_prevMousePosBeforeRaw);

    ::ShowCursor(false);

    Input::SetMouseRaw(true);
}

std::filesystem::path Window::OpenFileDialog()
{
    wchar_t filenameBuffer[512];
    filenameBuffer[0] = 0;

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"All Files\0*.*\0\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = filenameBuffer;
    ofn.nMaxFile = sizeof(filenameBuffer) / sizeof(filenameBuffer[0]);
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&ofn) == TRUE)
    {
        return filenameBuffer;
    }

    return {};
}

void Window::ResetIsResized()
{
    m_state.isWindowResized = false;
}

bool Window::IsMinimized() const
{
    return m_state.isWindowMinimized;
}

bool Window::IsClosed() const
{
    return m_state.isWindowClosed;
}

bool Window::IsResized() const
{
    return m_state.isWindowResized;
}

glm::uvec2 Window::GetWindowSize() const
{
    return m_state.windowSize;
}

glm::uvec2 Window::GetClientAreaSize() const
{
    return m_state.clientAreaSize;
}

HWND Window::GetWindowHandle() const
{
    return m_hwnd;
}

Window* Window::Get()
{
    return s_window;
}

WindowPtr Window::Create(std::wstring_view windowName, uint32_t width, uint32_t height)
{
    Assert(s_window == nullptr, "Window already created");

    WindowPtr window = std::make_unique<Window>(windowName, width, height);

    s_window = window.get();

    return window;
}

void Window::ResizeNewlyCreatedWindow(uint32_t width, uint32_t height)
{
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);

    RECT rcClient{}, rcWind{};
    POINT ptDiff{};
    GetClientRect(m_hwnd, &rcClient);
    GetWindowRect(m_hwnd, &rcWind);
    ptDiff.x = (rcWind.right - rcWind.left) - rcClient.right;
    ptDiff.y = (rcWind.bottom - rcWind.top) - rcClient.bottom;

    int newWidth = width + ptDiff.x;
    int newHeight = height + ptDiff.y;

    int x = ((mi.rcMonitor.right - mi.rcMonitor.left) - width) / 2;
    int y = ((mi.rcMonitor.bottom - mi.rcMonitor.top) - height) / 2;

    MoveWindow(m_hwnd, x, y, newWidth, newHeight, TRUE);
}

void Window::RegisterRawKeyboardDevice()
{
    RAWINPUTDEVICE keyboardDevice{};
    keyboardDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;
    keyboardDevice.usUsage = HID_USAGE_GENERIC_KEYBOARD;
    keyboardDevice.dwFlags = 0;
    keyboardDevice.hwndTarget = m_hwnd;

    if (!RegisterRawInputDevices(&keyboardDevice, 1, sizeof(keyboardDevice)))
    {
        LogError("Failed to register raw keyboard device");
        return;
    }
}

void Window::SetWindowHandle(HWND window)
{
    if (m_hwnd)
    {
        Error(m_hwnd, "m_hwnd already set");
    }

    m_hwnd = window;
}

bool Window::HandleWindowEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    bool isWindowEventHandled = false;

    if (WM_CLOSE == uMsg || WM_QUIT == uMsg)
    {
        ShowCursor();
        m_state.isWindowClosed = true;
        result = 0;
        isWindowEventHandled = true;
    }
    else if (HandleRawKeyboardWindowEvents(uMsg, wParam, lParam, result))
    {
        isWindowEventHandled = true;
    }
    else if (HandleInputCharWindowEvent(uMsg, wParam, lParam, result))
    {
        isWindowEventHandled = true;
    }
    else if (HandleMouseWindowEvents(uMsg, wParam, lParam, result))
    {
        isWindowEventHandled = true;
    }
    else if (HandleRawMouseWindowEvents(uMsg, wParam, lParam, result))
    {
        isWindowEventHandled = true;
    }
    else if (WM_SIZE == uMsg)
    {
        m_state.isWindowMinimized = wParam == SIZE_MINIMIZED;
        WindowResized(LOWORD(lParam), HIWORD(lParam));
    }
    else if (WM_ACTIVATE == uMsg)
    {
        if (WA_ACTIVE == wParam || WA_CLICKACTIVE == wParam)
        {
            m_isFocused = true;
        }
        else if (WA_INACTIVE == wParam)
        {
            m_isFocused = false;
        }
    }

    return isWindowEventHandled;
}

bool Window::HandleRawKeyboardWindowEvents(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    if (WM_INPUT == uMsg)
    {
        RAWINPUT rawInput{};

        UINT dwSize = sizeof(rawInput);
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &rawInput, &dwSize, sizeof(RAWINPUTHEADER));

        if (rawInput.header.dwType == RIM_TYPEKEYBOARD)
        {
            const RAWKEYBOARD& rawKeyboard = rawInput.data.keyboard;

            uint16_t scancode = rawKeyboard.MakeCode;
            if (scancode > s_keyboardScancodesToKeys.size())
            {
                return false;
            }

            Key key = s_keyboardScancodesToKeys[scancode];
            if (key == Key::None)
            {
                return false;
            }

            if (rawKeyboard.Flags & RI_KEY_BREAK)
            {
                Input::SetKeyReleased(key);
            }
            else if (!Input::IsKeyDown(key))
            {
                Input::SetKeyPressed(key);
            }

            result = 0;
            return true;
        }
    }

    return false;
}

bool Window::HandleInputCharWindowEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    if (WM_CHAR == uMsg)
    {
        if (wParam < ' ')
        {
            return false;
        }

        Input::AddCharTyped((wchar_t)wParam);

        result = 0;
        return true;
    }

    return false;
}

bool Window::HandleMouseWindowEvents(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    bool isMouseEvent = true;

    if (WM_LBUTTONDOWN == uMsg)
    {
        SetCapture(m_hwnd);
        Input::SetKeyPressed(Key::LButton);
        result = 0;
    }
    else if (WM_LBUTTONUP == uMsg)
    {
        ReleaseCapture();
        Input::SetKeyReleased(Key::LButton);
        result = 0;
    }
    else if (WM_RBUTTONDOWN == uMsg)
    {
        SetCapture(m_hwnd);
        Input::SetKeyPressed(Key::RButton);
        result = 0;
    }
    else if (WM_RBUTTONUP == uMsg)
    {
        ReleaseCapture();
        Input::SetKeyReleased(Key::RButton);
        result = 0;
    }
    else if (WM_MOUSEWHEEL == uMsg)
    {
        short s = HIWORD(wParam);
        float wheelDelta = s / 360.0f;
        Input::UpdateMouseWheelDelta(wheelDelta);
        result = 0;
    }
    else
    {
        isMouseEvent = false;
    }

    return isMouseEvent;
}

bool Window::HandleRawMouseWindowEvents(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    if (WM_INPUT != uMsg)
    {
        return false;
    }

    RAWINPUT rawInput{};

    UINT dwSize = sizeof(rawInput);
    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &rawInput, &dwSize, sizeof(RAWINPUTHEADER));

    if (rawInput.header.dwType == RIM_TYPEMOUSE)
    {
        const RAWMOUSE& rawMouse = rawInput.data.mouse;

        Input::UpdateMousePosRawDelta(MousePos(rawMouse.lLastX, rawMouse.lLastY));
        
        if (rawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
        {
            Input::SetKeyPressed(Key::LButton);
        }
        if (rawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
        {
            Input::SetKeyReleased(Key::LButton);
        }
        if (rawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
        {
            Input::SetKeyPressed(Key::LButton);
        }
        if (rawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
        {
            Input::SetKeyReleased(Key::LButton);
        }
        if (rawMouse.usButtonFlags & RI_MOUSE_WHEEL)
        {
            short wheelRotation = (short)rawMouse.usButtonData;
            Input::UpdateMouseWheelDelta((float)wheelRotation / 360.0f);
        }
    }

    result = 0;
    return true;
}

void Window::BeginPollEvents()
{
    Input::NewFrame();
}

void Window::EndPollEvents()
{
    if (!Input::IsMouseRaw())

    {
        SetMousePos();
    }
}

void Window::SetMousePos()
{
    POINT mousePos{};
    GetCursorPos(&mousePos);

    POINT topLeftPoint{}, bottomRigthPoint{ (LONG)m_state.clientAreaSize.x, (LONG)m_state.clientAreaSize.y };
    ClientToScreen(m_hwnd, &topLeftPoint);
    ClientToScreen(m_hwnd, &bottomRigthPoint);

    bool isOutOfScreen = mousePos.x < topLeftPoint.x || mousePos.x > bottomRigthPoint.x ||
        mousePos.y < topLeftPoint.y || mousePos.y > bottomRigthPoint.y;
    Input::SetMouseOutOfScreen(isOutOfScreen);
 
    int mouseX = mousePos.x;
    int mouseY = mousePos.y;
    mouseX -= topLeftPoint.x;
    mouseY -= topLeftPoint.y;

    Input::SetMousePos(MousePos(mouseX, mouseY));
}

void Window::WindowResized(uint32_t newWidth, uint32_t newHeight)
{
    m_state.clientAreaSize.x = newWidth;
    m_state.clientAreaSize.y = newHeight;
    
    RECT windowRect{};
    GetWindowRect(m_hwnd, &windowRect);

    m_state.windowSize.x = windowRect.right - windowRect.left;
    m_state.windowSize.y = windowRect.bottom - windowRect.top;

    m_state.isWindowResized = true;
}

LRESULT Window::HandleEvents(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Window* window = nullptr;

    if (WM_CREATE == uMsg)
    {
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
        window = (Window*)lpcs->lpCreateParams;
        window->SetWindowHandle(hWnd);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)window);
    }
    else
    {
        window = (Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }

    LRESULT result = 0;

    bool windowEventHandled = false;
    if (window)
    {
        windowEventHandled = window->HandleWindowEvent(uMsg, wParam, lParam, result);
    }

    if (!windowEventHandled)
    {
        result = DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return result;
}