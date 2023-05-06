#pragma once

#include "pch.hpp"

#include "CommandQueue.hpp"
#include "Game.hpp"
#include "MainWindow.hpp"
#include "Utils.hpp"

class Game;
class WindowClass;
class Window;

class Application
{
    UINT64 m_FrameCount = 0;

    WindowClass m_WindowClass;
    Window      m_Window;

    PResource m_BackBuffers[BUFFER_COUNT];

    bool     m_UseWarp      = false;
    uint32_t m_ClientWidth  = 1280;
    uint32_t m_ClientHeight = 720;

    // Window rectangle (used to toggle fullscreen state).
    RECT m_WindowRect;

    // DirectX 12 Objects
    PDevice         m_Device;
    PSwapChain      m_SwapChain;
    PDescriptorHeap m_RTVDescriptorHeap;
    UINT            m_RTVDescriptorSize;
    UINT            m_CurrentBackBufferIndex;

    // Synchronization objects
    UINT64 m_FrameFenceValues[BUFFER_COUNT] = {};

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

    void Flush()
    {
        m_CommandQueueDirect->Flush();
        m_CommandQueueCompute->Flush();
        m_CommandQueueCopy->Flush();
    }

    int Run(int nShowCmd);

    PDevice GetDevice() { return m_Device; }

    CommandQueue &GetCommandQueueDirect() noexcept { return *m_CommandQueueDirect; }
    CommandQueue &GetCommandQueueCompute() noexcept { return *m_CommandQueueCompute; }
    CommandQueue &GetCommandQueueCopy() noexcept { return *m_CommandQueueCopy; }

    constexpr UINT CurrentBackBufferIndex() const { return m_CurrentBackBufferIndex; }
    PResource      CurrentBackBuffer() const { return m_BackBuffers[m_CurrentBackBufferIndex]; }
    PSwapChain     SwapChain() const { return m_SwapChain; }

    CD3DX12_CPU_DESCRIPTOR_HANDLE CurrentRTV() const
    {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(
            m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_CurrentBackBufferIndex, m_RTVDescriptorSize);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE BackRTV() const
    {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(
            m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), BUFFER_COUNT, m_RTVDescriptorSize);
    }

    UINT Present()
    {
        UINT syncInterval = m_VSync ? 1 : 0;
        UINT presentFlags = m_TearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
        Assert(m_SwapChain->Present(syncInterval, presentFlags));
        m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

        return m_CurrentBackBufferIndex;
    }

    UINT64 GetFrameCount() const noexcept { return m_FrameCount; }
};
