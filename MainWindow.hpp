#pragma once

#include <Windows.h>
#include <exception>
#include <string>

class WindowClass
{
    HINSTANCE hInstance;
    LPCWSTR name;

  public:
    WindowClass(HINSTANCE hInstance, LPCWSTR name, WNDPROC lpfnWndProc) : hInstance(hInstance), name(name)
    {
        WNDCLASSEXW cls = {};
        cls.cbSize = sizeof(cls);
        cls.style = CS_HREDRAW | CS_VREDRAW;
        cls.lpfnWndProc = lpfnWndProc;
        cls.cbClsExtra = 0;
        cls.cbWndExtra = 0;
        cls.hInstance = hInstance;
        cls.hIcon = LoadIcon(hInstance, nullptr);
        cls.hCursor = LoadCursor(hInstance, IDC_ARROW);
        cls.hbrBackground = (HBRUSH)COLOR_WINDOW;
        cls.lpszMenuName = nullptr;
        cls.lpszClassName = name;
        cls.hIconSm = cls.hIcon;

        ATOM atom = RegisterClassExW(&cls);
        if (!atom)
            throw std::exception();
    }

    WindowClass(const WindowClass &) = delete;
    WindowClass &operator=(const WindowClass &) = delete;

    ~WindowClass()
    {
        UnregisterClassW(name, hInstance);
    }

    constexpr HINSTANCE HInstance() const noexcept
    {
        return hInstance;
    }

    constexpr LPCWSTR Name() const noexcept
    {
        return name;
    }
};

class Window
{
    HWND hWnd;
    HINSTANCE hInstance;

  public:
    Window(const WindowClass &cls, LPWSTR title, int width, int height) : hInstance(cls.HInstance())
    {
        int screenSizeX = GetSystemMetrics(SM_CXSCREEN);
        int screenSizeY = GetSystemMetrics(SM_CYSCREEN);
        RECT windowRect;
        windowRect.left = (screenSizeX - width) / 2;
        windowRect.top = (screenSizeY - height) / 2;
        windowRect.right = windowRect.left + width;
        windowRect.bottom = windowRect.top + height;
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        hWnd = CreateWindowExW(0, cls.Name(), title, WS_OVERLAPPEDWINDOW, windowRect.left, windowRect.top,
                               windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, HWND_DESKTOP,
                               nullptr, hInstance, nullptr);
        if (!hWnd)
        {
            DWORD err = GetLastError();
            throw std::exception(std::to_string(err).c_str());
        }
    }

    ~Window()
    {
        DestroyWindow(hWnd);
    }

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    constexpr HWND HWnd() const noexcept
    {
        return hWnd;
    }
};