#pragma once

#include <Windows.h>
#include <exception>
#include <string>

class Application;

class WindowClass
{
    HINSTANCE m_hInstance;
    LPCWSTR   m_Name;

  public:
    WindowClass(HINSTANCE hInstance, LPCWSTR name, WNDPROC lpfnWndProc)
        : m_hInstance(hInstance),
          m_Name(name)
    {
        WNDCLASSEXW cls   = {};
        cls.cbSize        = sizeof(cls);
        cls.style         = CS_HREDRAW | CS_VREDRAW;
        cls.lpfnWndProc   = lpfnWndProc;
        cls.cbClsExtra    = 0;
        cls.cbWndExtra    = 0;
        cls.hInstance     = hInstance;
        cls.hIcon         = nullptr; // LoadIconW(hInstance, 0);
        cls.hCursor       = LoadCursor(hInstance, IDC_ARROW);
        cls.hbrBackground = (HBRUSH)COLOR_WINDOW;
        cls.lpszMenuName  = nullptr;
        cls.lpszClassName = name;
        cls.hIconSm       = cls.hIcon;

        ATOM atom = RegisterClassExW(&cls);
        if (!atom)
            throw std::exception();
    }

    WindowClass(const WindowClass &)            = delete;
    WindowClass &operator=(const WindowClass &) = delete;

    ~WindowClass() { UnregisterClassW(m_Name, m_hInstance); }

    constexpr HINSTANCE HInstance() const noexcept { return m_hInstance; }
    constexpr LPCWSTR   Name() const noexcept { return m_Name; }
};

class Window
{
    HWND      m_hWnd;
    HINSTANCE m_hInstance;

  public:
    Window(const WindowClass &cls, LPCWSTR title, int width, int height)
        : m_hInstance(cls.HInstance())
    {
        int  screenSizeX  = GetSystemMetrics(SM_CXSCREEN);
        int  screenSizeY  = GetSystemMetrics(SM_CYSCREEN);
        RECT windowRect   = {};
        windowRect.left   = (screenSizeX - width) / 2;
        windowRect.top    = (screenSizeY - height) / 2;
        windowRect.right  = windowRect.left + width;
        windowRect.bottom = windowRect.top + height;
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        m_hWnd = CreateWindowExW(0,
                                 cls.Name(),
                                 title,
                                 WS_OVERLAPPEDWINDOW,
                                 windowRect.left,
                                 windowRect.top,
                                 windowRect.right - windowRect.left,
                                 windowRect.bottom - windowRect.top,
                                 HWND_DESKTOP,
                                 nullptr,
                                 m_hInstance,
                                 nullptr);
        if (!m_hWnd)
        {
            DWORD err = GetLastError();
            throw std::runtime_error(std::to_string(err));
        }
    }

    ~Window() { DestroyWindow(m_hWnd); }

    Window(const Window &)            = delete;
    Window &operator=(const Window &) = delete;

    constexpr HWND Get() const noexcept { return m_hWnd; }
};
