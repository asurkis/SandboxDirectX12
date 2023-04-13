#pragma once

#include "Commons.hpp"
#include "MainWindow.hpp"

class Game
{
    WindowClass m_WindowClass;
    Window      m_Window;
    int         m_Width;
    int         m_Height;
    bool        m_VSync;

  public:
    Game(const std::wstring &name, int width, int height, bool VSync)
        : m_WindowClass()
    {
    }

    ~Game()
    {
    }
};