#pragma once

#include "CommandQueue.hpp"
#include "Commons.hpp"
#include "Device.hpp"
#include "Game.hpp"
#include "MainWindow.hpp"
#include "Utils.hpp"

#include <algorithm>
#include <chrono>
#include <sstream>

class Game;

class Application
{
    WindowClass m_WindowClass;
    Window      m_Window;

    static const UINT BufferCount = 3;
    PResource         g_BackBuffers[BufferCount];

    bool     m_UseWarp      = false;
    uint32_t m_ClientWidth  = 1280;
    uint32_t m_ClientHeight = 720;

    // Window rectangle (used to toggle fullscreen state).
    RECT m_WindowRect;

    // DirectX 12 Objects
    PSwapChain           m_SwapChain;
    PGraphicsCommandList m_CommandList;
    PCommandAllocator    m_CommandAllocators[BufferCount];
    PDescriptorHeap      m_RTVDescriptorHeap;
    UINT                 m_RTVDescriptorSize;
    UINT                 m_CurrentBackBufferIndex;

    // Synchronization objects
    uint64_t m_FrameFenceValues[BufferCount] = {};

    // By default, enable V-Sync.
    // Can be toggled with the V key.
    bool m_VSync            = true;
    bool m_TearingSupported = false;
    // By default, use windowed mode.
    // Can be toggled with the Alt+Enter or F11
    bool m_Fullscreen = false;

    std::optional<Device>       m_Device;
    std::optional<CommandQueue> m_CommandQueueDirect;
    std::optional<CommandQueue> m_CommandQueueCompute;
    std::optional<CommandQueue> m_CommandQueueCopy;

    inline static Application *instance = nullptr;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (!instance) return DefWindowProcW(hWnd, uMsg, wParam, lParam);

        switch (uMsg)
        {
        case WM_DESTROY: PostQuitMessage(0); break;

        case WM_PAINT:
            instance->Update();
            instance->Render();
            break;

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {
            bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

            switch (wParam)
            {
            case 'V': instance->m_VSync ^= true; break;

            case VK_ESCAPE: PostQuitMessage(0); break;

            case VK_RETURN:
                if (alt) instance->ToggleFullscreen();
                break;

            case VK_F11: instance->ToggleFullscreen(); break;
            }
            break;
        }

        case WM_SYSCHAR: break;

        case WM_SIZE: {
            RECT cr = {};
            GetClientRect(instance->m_Window.HWnd(), &cr);
            LONG width  = cr.right - cr.left;
            LONG height = cr.bottom - cr.top;
            instance->Resize(width, height);
            break;
        }

        default: return DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }

        return 0;
    }

    void ParseCommandLineArguments()
    {
        int       argc;
        wchar_t **argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

        for (size_t i = 0; i < argc; ++i)
        {
            if (wcscmp(argv[i], L"-w") == 0 || wcscmp(argv[i], L"--width") == 0)
            {
                m_ClientWidth = ::wcstol(argv[++i], nullptr, 10);
            }
            if (wcscmp(argv[i], L"-h") == 0 || wcscmp(argv[i], L"--height") == 0)
            {
                m_ClientHeight = ::wcstol(argv[++i], nullptr, 10);
            }
            if (wcscmp(argv[i], L"-warp") == 0 || wcscmp(argv[i], L"--warp") == 0) { m_UseWarp = true; }
        }

        // Free memory allocated by CommandLineToArgvW
        ::LocalFree(argv);
    }

    void EnableDebugLayer()
    {
#ifdef _DEBUG
        // Always enable the debug layer before doing anything DX12 related
        // so all possible errors generated while creating DX12 objects
        // are caught by the debug layer.
        ComPtr<ID3D12Debug> debugInterface;
        Assert(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
        debugInterface->EnableDebugLayer();
#endif
    }

    ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp)
    {
        UINT createFactoryFlags = 0;
#ifdef _DEBUG
        createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
        ComPtr<IDXGIFactory4> factory;
        Assert(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&factory)));
        ComPtr<IDXGIAdapter1> adapter1;
        ComPtr<IDXGIAdapter4> adapter4;
        if (useWarp)
        {
            Assert(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter1)));
            Assert(adapter1.As(&adapter4));
            return adapter4;
        }
        SIZE_T maxDedicatedVram = 0;
        for (UINT i = 0; factory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 desc1;
            adapter1->GetDesc1(&desc1);
            if (desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
            if (FAILED(D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr)))
                continue;
            if (desc1.DedicatedVideoMemory < maxDedicatedVram) continue;
            maxDedicatedVram = desc1.DedicatedVideoMemory;
            Assert(adapter1.As(&adapter4));
        }
        return adapter4;
    }

    bool CheckTearingSupport()
    {
        ComPtr<IDXGIFactory5> factory5;
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory5)))) return false;
        BOOL allowTearing = FALSE;
        if (FAILED(
                factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
            return false;
        return allowTearing;
    }

    void UpdateRenderTargetViews(PDevice device, PSwapChain swapChain, PDescriptorHeap heap)
    {
        UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(heap->GetCPUDescriptorHandleForHeapStart());
        for (UINT i = 0; i < BufferCount; ++i)
        {
            PResource backBuffer;
            Assert(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
            device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
            g_BackBuffers[i] = backBuffer;
            rtvHandle.Offset(rtvDescriptorSize);
        }
    }

    void Update()
    {
        static uint64_t                           frameCounter = 0;
        static std::chrono::high_resolution_clock clock;
        static auto                               t0 = clock.now();

        ++frameCounter;
        auto                          t1 = clock.now();
        std::chrono::duration<double> dt = t1 - t0;
        // t0 = t1;

        double elapsedSeconds = dt.count();
        if (elapsedSeconds > 1.0)
        {
            std::wstringstream ss;
            ss << L"FPS: " << frameCounter / elapsedSeconds << '\n';
            OutputDebugStringW(ss.str().c_str());
            frameCounter = 0;
            t0           = t1;
        }
    }

    void Render()
    {
        auto &&commandAllocator = m_CommandAllocators[m_CurrentBackBufferIndex];
        auto &&backBuffer       = g_BackBuffers[m_CurrentBackBufferIndex];
        commandAllocator->Reset();
        {
            m_CommandList->Reset(commandAllocator.Get(), nullptr);
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
            m_CommandList->ResourceBarrier(1, &barrier);

            FLOAT                         clearColor[] = {0.4f, 0.6f, 0.9f, 1.0f};
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                                              m_CurrentBackBufferIndex,
                                              m_RTVDescriptorSize);
            m_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
        }
        {
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            m_CommandList->ResourceBarrier(1, &barrier);
            m_CommandQueueDirect->ExecuteCommandList(m_CommandList.Get());

            UINT syncInterval = m_VSync ? 1 : 0;
            UINT presentFlags = m_TearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
            Assert(m_SwapChain->Present(syncInterval, presentFlags));
            m_FrameFenceValues[m_CurrentBackBufferIndex] = m_CommandQueueDirect->Signal();

            m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
            m_CommandQueueDirect->WaitForFenceValue(m_FrameFenceValues[m_CurrentBackBufferIndex]);
        }
    }

    void Resize(UINT32 width, UINT32 height)
    {
        if (m_ClientWidth == width && m_ClientHeight == height) return;
        m_ClientWidth  = (std::max)(1u, width);
        m_ClientHeight = (std::max)(1u, height);
        m_CommandQueueDirect->Flush();
        for (UINT i = 0; i < BufferCount; ++i)
        {
            g_BackBuffers[i].Reset();
            m_FrameFenceValues[i] = m_FrameFenceValues[m_CurrentBackBufferIndex];
        }

        DXGI_SWAP_CHAIN_DESC desc = {};
        Assert(m_SwapChain->GetDesc(&desc));
        Assert(
            m_SwapChain->ResizeBuffers(BufferCount, m_ClientWidth, m_ClientHeight, desc.BufferDesc.Format, desc.Flags));
        m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
        UpdateRenderTargetViews(m_Device->Get(), m_SwapChain, m_RTVDescriptorHeap);
    }

    void ToggleFullscreen() { SetFullscreen(!m_Fullscreen); }

    void SetFullscreen(bool fullscreen)
    {
        if (m_Fullscreen == fullscreen) return;
        m_Fullscreen = fullscreen;
        if (m_Fullscreen)
        {
            GetWindowRect(m_Window.HWnd(), &m_WindowRect);
            UINT windowStyle = WS_OVERLAPPED;
            SetWindowLongW(m_Window.HWnd(), GWL_STYLE, windowStyle);

            HMONITOR      hMonitor    = MonitorFromWindow(m_Window.HWnd(), MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitorInfo = {};
            monitorInfo.cbSize        = sizeof(MONITORINFOEX);
            GetMonitorInfoW(hMonitor, &monitorInfo);

            SetWindowPos(m_Window.HWnd(),
                         HWND_TOP,
                         monitorInfo.rcMonitor.left,
                         monitorInfo.rcMonitor.top,
                         monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                         monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                         SWP_FRAMECHANGED | SWP_NOACTIVATE);
            ShowWindow(m_Window.HWnd(), SW_MAXIMIZE);
        }
        else
        {
            SetWindowLong(m_Window.HWnd(), GWL_STYLE, WS_OVERLAPPEDWINDOW);
            SetWindowPos(m_Window.HWnd(),
                         HWND_NOTOPMOST,
                         m_WindowRect.left,
                         m_WindowRect.top,
                         m_WindowRect.right - m_WindowRect.left,
                         m_WindowRect.bottom - m_WindowRect.top,
                         SWP_FRAMECHANGED | SWP_NOACTIVATE);
            ShowWindow(m_Window.HWnd(), SW_NORMAL);
        }
    }

  public:
    Application(HINSTANCE hInstance)
        : m_WindowClass(hInstance, L"DX12WindowClass", &Application::WndProc),
          m_Window(m_WindowClass, L"Main Window Title", 1280, 720)
    {
        ParseCommandLineArguments();
        EnableDebugLayer();

        m_TearingSupported = CheckTearingSupport();

        GetWindowRect(m_Window.HWnd(), &m_WindowRect);

        ComPtr<IDXGIAdapter4> adapter4 = GetAdapter(m_UseWarp);

        m_Device.emplace(adapter4);
        m_CommandQueueDirect.emplace(m_Device->Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
        m_CommandQueueCompute.emplace(m_Device->Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE);
        m_CommandQueueCopy.emplace(m_Device->Get(), D3D12_COMMAND_LIST_TYPE_COPY);
        m_SwapChain = m_CommandQueueDirect->CreateSwapChain(
            m_Window.HWnd(), m_TearingSupported, m_ClientWidth, m_ClientHeight, BufferCount);
        m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
        m_RTVDescriptorHeap      = m_Device->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, BufferCount);
        m_RTVDescriptorSize      = m_Device->Get()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        UpdateRenderTargetViews(m_Device->Get(), m_SwapChain, m_RTVDescriptorHeap);

        for (int i = 0; i < BufferCount; ++i)
            m_CommandAllocators[i] = m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
        m_CommandList = m_Device->CreateCommandList(m_CommandAllocators[m_CurrentBackBufferIndex],
                                                    D3D12_COMMAND_LIST_TYPE_DIRECT);
    }

    ~Application()
    {
        m_CommandQueueDirect->Flush();
        m_CommandQueueCompute->Flush();
        m_CommandQueueCopy->Flush();
    }

    int Run(int nShowCmd)
    {
        instance = this;
        Game game(this, m_ClientWidth, m_ClientHeight, m_VSync);
        ShowWindow(m_Window.HWnd(), nShowCmd);

        MSG msg;
        while (GetMessageW(&msg, HWND_DESKTOP, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        instance = nullptr;
        return msg.wParam;
    }

    PDevice GetDevice() { return m_Device->Get(); }

    CommandQueue &GetCommandQueue(D3D12_COMMAND_LIST_TYPE type)
    {
        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_DIRECT: return *m_CommandQueueDirect;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: return *m_CommandQueueCompute;
        case D3D12_COMMAND_LIST_TYPE_COPY: return *m_CommandQueueCopy;
        default: throw std::runtime_error("No such queue");
        }
    }
};
