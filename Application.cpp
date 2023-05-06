#include "Application.hpp"

Application::Application(HINSTANCE hInstance)
    : m_WindowClass(hInstance, L"DX12WindowClass", &Application::WndProc),
      m_Window(m_WindowClass, L"Main Window Title", 1280, 720)
{
    EnableDebugLayer();

    m_TearingSupported = CheckTearingSupport();

    GetWindowRect(m_Window.Get(), &m_WindowRect);

    PFactory factory = CreateFactory();
    m_Device         = CreateDevice(factory, m_UseWarp);
    m_CommandQueueDirect.emplace(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_CommandQueueCompute.emplace(m_Device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
    m_CommandQueueCopy.emplace(m_Device, D3D12_COMMAND_LIST_TYPE_COPY);
    m_SwapChain = m_CommandQueueDirect->CreateSwapChain(
        factory, m_Window.Get(), m_TearingSupported, m_ClientWidth, m_ClientHeight, BUFFER_COUNT);
    m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC dhDesc = {};
    dhDesc.NumDescriptors             = BUFFER_COUNT + 1;
    dhDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    Assert(m_Device->CreateDescriptorHeap(&dhDesc, IID_PPV_ARGS(&m_RTVDescriptorHeap)));
    m_RTVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    UpdateRenderTargetViews();
}

LRESULT CALLBACK Application::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!g_Instance)
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_DESTROY: PostQuitMessage(0); break;

    case WM_PAINT:
        g_Instance->m_FrameCount++;
        g_Instance->m_Game->OnUpdate();
        g_Instance->m_Game->OnRender();
        break;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN: {
        bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

        switch (wParam)
        {
        case 'V': g_Instance->m_VSync ^= true; break;

        case VK_ESCAPE: PostQuitMessage(0); break;

        case VK_RETURN:
            if (alt)
                g_Instance->ToggleFullscreen();
            break;

        case VK_F11: g_Instance->ToggleFullscreen(); break;

        case VK_OEM_MINUS: g_Instance->m_Game->m_FovStep--; break;
        case VK_OEM_PLUS: g_Instance->m_Game->m_FovStep++; break;
        case '0': g_Instance->m_Game->m_FovStep = 0; break;
        case VK_SPACE: g_Instance->m_Game->m_ShakeStrength = 1.0; break;
        case 'Z': g_Instance->m_Game->m_ZLess ^= true; break;

        case 'R': g_Instance->m_Game->ReloadShaders(); break;
        }
        break;
    }

    case WM_SYSCHAR: break;

    case WM_SIZE: {
        RECT cr = {};
        GetClientRect(g_Instance->m_Window.Get(), &cr);
        LONG width  = cr.right - cr.left;
        LONG height = cr.bottom - cr.top;
        g_Instance->OnResize(width, height);
        break;
    }

    default: return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

void Application::EnableDebugLayer()
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

PFactory Application::CreateFactory()
{
    UINT createFactoryFlags = 0;
#ifdef _DEBUG
    createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
    PFactory result;
    Assert(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&result)));
    return result;
}

PDevice Application::CreateDevice(PFactory factory, bool useWarp)
{
    PDevice result;

    if (useWarp)
    {
        ComPtr<IDXGIAdapter> adapter;
        Assert(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
        D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&result));
        return result;
    }

    ComPtr<IDXGIAdapter1> adapter1;
    ComPtr<IDXGIAdapter4> adapter4;
    SIZE_T                maxDedicatedVram = 0;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc1;
        adapter1->GetDesc1(&desc1);
        if (desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;
        if (FAILED(D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr)))
            continue;
        if (desc1.DedicatedVideoMemory < maxDedicatedVram)
            continue;
        maxDedicatedVram = desc1.DedicatedVideoMemory;
        Assert(adapter1.As(&adapter4));
    }

    D3D12CreateDevice(adapter4.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&result));
    return result;
}

bool Application::CheckTearingSupport()
{
    ComPtr<IDXGIFactory5> factory5;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory5))))
        return false;
    BOOL allowTearing = FALSE;
    if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
        return false;
    return allowTearing;
}

void Application::UpdateRenderTargetViews()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < BUFFER_COUNT; ++i)
    {
        PResource backBuffer;
        Assert(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
        m_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
        m_BackBuffers[i] = backBuffer;
        rtvHandle.Offset(m_RTVDescriptorSize);
    }
}

void Application::OnResize(UINT32 width, UINT32 height)
{
    if (m_ClientWidth == width && m_ClientHeight == height)
        return;
    m_ClientWidth  = (std::max)(1u, width);
    m_ClientHeight = (std::max)(1u, height);
    m_CommandQueueDirect->Flush();
    for (UINT i = 0; i < BUFFER_COUNT; ++i)
    {
        m_BackBuffers[i].Reset();
        m_FrameFenceValues[i] = m_FrameFenceValues[m_CurrentBackBufferIndex];
    }

    DXGI_SWAP_CHAIN_DESC desc = {};
    Assert(m_SwapChain->GetDesc(&desc));
    Assert(m_SwapChain->ResizeBuffers(BUFFER_COUNT, m_ClientWidth, m_ClientHeight, desc.BufferDesc.Format, desc.Flags));
    m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
    UpdateRenderTargetViews();

    m_Game->OnResize(width, height);
}

int Application::Run(int nShowCmd)
{
    g_Instance = this;
    m_Game.emplace(this, m_ClientWidth, m_ClientHeight);
    ShowWindow(m_Window.Get(), nShowCmd);

    MSG msg;
    while (GetMessageW(&msg, HWND_DESKTOP, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    m_Game.reset();
    g_Instance = nullptr;
    return static_cast<int>(msg.wParam);
}

void Application::SetFullscreen(bool fullscreen)
{
    if (m_Fullscreen == fullscreen)
        return;
    m_Fullscreen = fullscreen;
    if (m_Fullscreen)
    {
        GetWindowRect(m_Window.Get(), &m_WindowRect);
        UINT windowStyle = WS_OVERLAPPED;
        SetWindowLongW(m_Window.Get(), GWL_STYLE, windowStyle);

        HMONITOR      hMonitor    = MonitorFromWindow(m_Window.Get(), MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX monitorInfo = {};
        monitorInfo.cbSize        = sizeof(MONITORINFOEX);
        GetMonitorInfoW(hMonitor, &monitorInfo);

        SetWindowPos(m_Window.Get(),
                     HWND_TOP,
                     monitorInfo.rcMonitor.left,
                     monitorInfo.rcMonitor.top,
                     monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                     monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                     SWP_FRAMECHANGED | SWP_NOACTIVATE);
        ShowWindow(m_Window.Get(), SW_MAXIMIZE);
    }
    else
    {
        SetWindowLong(m_Window.Get(), GWL_STYLE, WS_OVERLAPPEDWINDOW);
        SetWindowPos(m_Window.Get(),
                     HWND_NOTOPMOST,
                     m_WindowRect.left,
                     m_WindowRect.top,
                     m_WindowRect.right - m_WindowRect.left,
                     m_WindowRect.bottom - m_WindowRect.top,
                     SWP_FRAMECHANGED | SWP_NOACTIVATE);
        ShowWindow(m_Window.Get(), SW_NORMAL);
    }
}
