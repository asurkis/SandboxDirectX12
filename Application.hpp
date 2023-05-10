#pragma once

#include "pch.hpp"

#include "Game.hpp"
#include "MyDXLib/CommandQueue.hpp"
#include "MyDXLib/MainWindow.hpp"
#include "MyDXLib/Utils.hpp"

class Game;

class Application
{
    UINT64 m_FrameCount = 0;

    WindowClass m_WindowClass;
    Window      m_Window;

    PResource m_BackBuffers[BACK_BUFFER_COUNT];

    bool     m_UseWarp      = false;
    uint32_t m_ClientWidth  = 1280;
    uint32_t m_ClientHeight = 720;

    // Window rectangle (used to toggle fullscreen state).
    RECT m_WindowRect;

    // DirectX 12 Objects
    PDevice    m_Device;
    PSwapChain m_SwapChain;
    UINT       m_CurrentBackBufferIndex;

    std::optional<DescriptorHeap> m_RTVDescriptorHeap;

    // By default, enable V-Sync.
    // Can be toggled with the V key.
    bool m_VSync            = true;
    bool m_TearingSupported = false;
    // By default, use windowed mode.
    // Can be toggled with the Alt+Enter or F11
    bool m_Fullscreen = false;

    std::optional<CommandQueue> m_CommandQueueDirect;
    std::optional<CommandQueue> m_CommandQueueCompute;
    std::optional<CommandQueue> m_CommandQueueCopy;

    std::optional<Game> m_Game;

    inline static Application *g_Instance = nullptr;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void             EnableDebugLayer();
    static PFactory         CreateFactory();
    static PDevice          CreateDevice(PFactory factory, bool useWarp);
    static bool             CheckTearingSupport();

    void UpdateRenderTargetViews();
    void OnResize(UINT32 width, UINT32 height);

    void ToggleFullscreen() { SetFullscreen(!m_Fullscreen); }
    void SetFullscreen(bool fullscreen);

  public:
    static Application *Get() noexcept { return g_Instance; }
    Application(HINSTANCE hInstance);
    ~Application() { Flush(); }

    PDevice GetDevice() const noexcept { return m_Device; }

    CommandQueue &GetCommandQueueDirect() noexcept { return *m_CommandQueueDirect; }
    CommandQueue &GetCommandQueueCompute() noexcept { return *m_CommandQueueCompute; }
    CommandQueue &GetCommandQueueCopy() noexcept { return *m_CommandQueueCopy; }

    UINT64     GetFrameCount() const noexcept { return m_FrameCount; }
    UINT       GetCurrentBackBufferIndex() const noexcept { return m_CurrentBackBufferIndex; }
    PResource  GetCurrentBackBuffer() const { return m_BackBuffers[m_CurrentBackBufferIndex]; }
    PSwapChain GetSwapChain() const { return m_SwapChain; }

    D3D12_CPU_DESCRIPTOR_HANDLE CurrentRTV() const
    {
        return m_RTVDescriptorHeap->GetCpuHandle(BACK_BUFFER_START + m_CurrentBackBufferIndex);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE IntermediateRTV() const
    {
        return m_RTVDescriptorHeap->GetCpuHandle(INTERMEDIATE_RTV_START);
    }

    void Flush()
    {
        m_CommandQueueDirect->Flush();
        m_CommandQueueCompute->Flush();
        m_CommandQueueCopy->Flush();
    }

    int  Run(int nShowCmd);
    UINT Present();
};
