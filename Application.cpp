#include "Application.hpp"

#include <windowsx.h>

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
        factory, m_Window.Get(), m_TearingSupported, m_ClientWidth, m_ClientHeight, BACK_BUFFER_COUNT);
    m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

    m_RTVDescriptorHeap.emplace(
        m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, RTV_COUNT);

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
        case VK_SPACE: g_Instance->m_Game->m_ShakeStrength = 1.0; break;
        case '0': g_Instance->m_Game->m_FovStep = 0; break;
        case 'Z': g_Instance->m_Game->m_ZLess ^= true; break;
        case 'R':
            try
            {
                g_Instance->m_Game->ReloadShaders();
            }
            catch (const std::runtime_error &e)
            {
                OutputDebugStringA(e.what());
            }
            break;

        case 'W': g_Instance->m_Game->m_MoveForward = true; break;
        case 'A': g_Instance->m_Game->m_MoveLeft = true; break;
        case 'S': g_Instance->m_Game->m_MoveBack = true; break;
        case 'D': g_Instance->m_Game->m_MoveRight = true; break;
        }
        break;
    }

    case WM_SYSKEYUP:
    case WM_KEYUP:
        switch (wParam)
        {
        case 'W': g_Instance->m_Game->m_MoveForward = false; break;
        case 'A': g_Instance->m_Game->m_MoveLeft = false; break;
        case 'S': g_Instance->m_Game->m_MoveBack = false; break;
        case 'D': g_Instance->m_Game->m_MoveRight = false; break;
        }
        break;

    case WM_SYSCHAR: break;

    case WM_SIZE: {
        RECT cr = {};
        GetClientRect(g_Instance->m_Window.Get(), &cr);
        LONG width  = cr.right - cr.left;
        LONG height = cr.bottom - cr.top;
        g_Instance->OnResize(width, height);
        break;
    }

    case WM_LBUTTONDOWN: g_Instance->m_Game->OnMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); break;

    case WM_MOUSEMOVE:
        if ((wParam & MK_LBUTTON) == 0)
            break;
        g_Instance->m_Game->OnMouseDrag(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;

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

    Assert(D3D12CreateDevice(adapter4.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&result)));

#ifdef _DEBUG
    // Enable debug messages in debug mode.
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(result.As(&pInfoQueue)))
    {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);

        // Suppress whole categories of messages
        // D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] = {D3D12_MESSAGE_SEVERITY_INFO};

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE, // I'm really not sure how to avoid this
                                                                          // message.
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,   // This warning occurs when using capture frame while graphics
                                                      // debugging.
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE, // This warning occurs when using capture frame while graphics
                                                      // debugging.

            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            // Workarounds for debug layer issues on hybrid-graphics systems
            D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,

            // For reversing Z in real time
            D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
        };

        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        // NewFilter.DenyList.NumCategories = _countof(Categories);
        // NewFilter.DenyList.pCategoryList = Categories;
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs        = _countof(DenyIds);
        NewFilter.DenyList.pIDList       = DenyIds;

        Assert(pInfoQueue->PushStorageFilter(&NewFilter));
    }
#endif

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
    for (UINT i = 0; i < BACK_BUFFER_COUNT; ++i)
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVDescriptorHeap->GetCpuHandle(i));
        PResource                     backBuffer;
        Assert(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
        m_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
        m_BackBuffers[i] = backBuffer;
    }
}

void Application::OnResize(UINT32 width, UINT32 height)
{
    if (m_ClientWidth == width && m_ClientHeight == height)
        return;
    m_ClientWidth  = (std::max)(1u, width);
    m_ClientHeight = (std::max)(1u, height);
    m_CommandQueueDirect->Flush();
    for (UINT i = 0; i < BACK_BUFFER_COUNT; ++i)
        m_BackBuffers[i].Reset();

    DXGI_SWAP_CHAIN_DESC desc = {};
    Assert(m_SwapChain->GetDesc(&desc));
    Assert(m_SwapChain->ResizeBuffers(
        BACK_BUFFER_COUNT, m_ClientWidth, m_ClientHeight, desc.BufferDesc.Format, desc.Flags));
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

UINT Application::Present()
{
    UINT syncInterval = m_VSync ? 1 : 0;
    UINT presentFlags = m_TearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    Assert(m_SwapChain->Present(syncInterval, presentFlags));
    m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
    return m_CurrentBackBufferIndex;
}
