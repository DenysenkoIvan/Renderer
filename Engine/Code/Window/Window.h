#pragma once

#include <condition_variable>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include <Windows.h>

#include <Input/Input.h>

namespace
{
    struct PrevWindowState
    {
        DWORD style = 0;
        WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };
    };

    struct WindowState
    {
        bool isFullscreen = false;
        PrevWindowState prevState{};
        bool isWindowMinimized = false;
        bool isWindowClosed = false;
        bool isWindowResized = false;
        glm::uvec2 windowSize = { 0, 0 };
        glm::uvec2 clientAreaSize = { 0, 0 };
    };
}

class Window;
using WindowPtr = std::unique_ptr<Window>;

class Window
{
public:
    Window(std::wstring_view windowName, uint32_t width, uint32_t height);
    ~Window();

    void PollEvents();

    void SetFocused();
    bool IsInFocus() const;

    void EnterFullscreenBorderless();
    void ExitFullscreenBorderless();

    void ShowCursor();
    void HideCursor();

    std::filesystem::path OpenFileDialog();

    void ResetIsResized();

    bool IsMinimized() const;
    bool IsClosed() const;
    bool IsResized() const;
    glm::uvec2 GetWindowSize() const;
    glm::uvec2 GetClientAreaSize() const;

    HWND GetWindowHandle() const;

    static Window* Get();
    static WindowPtr Create(std::wstring_view windowName, uint32_t width, uint32_t height);

private:
    void ResizeNewlyCreatedWindow(uint32_t width, uint32_t height);
    void RegisterRawKeyboardDevice();
    void SetWindowHandle(HWND window);

    bool HandleWindowEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
    bool HandleRawKeyboardWindowEvents(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
    bool HandleInputCharWindowEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
    bool HandleMouseWindowEvents(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
    bool HandleRawMouseWindowEvents(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);

    void BeginPollEvents();
    void EndPollEvents();
    void SetMousePos();

    void WindowResized(uint32_t newWidth, uint32_t newHeight);

    static LRESULT HandleEvents(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    HWND m_hwnd{ 0 };
    WindowState m_state{};
    POINT m_prevMousePosBeforeRaw;
    bool m_isFocused = false;

    static Window* s_window;
};