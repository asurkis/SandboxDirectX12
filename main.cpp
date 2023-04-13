#define _DEBUG
// #define WIN32_LEAN_AND_MEAN
#include "Commons.hpp"
#include "MainWindow.hpp"
#include "Utils.hpp"

#include <algorithm>
#include <chrono>
#include <sstream>

#include <d3dcompiler.h>

const UINT g_NumFrames = 3;
PResource  g_BackBuffers[g_NumFrames];

bool     g_UseWarp      = false;
uint32_t g_ClientWidth  = 1280;
uint32_t g_ClientHeight = 720;

bool g_IsInitialized = false;

// Window handle.
HWND g_hWnd;
// Window rectangle (used to toggle fullscreen state).
RECT g_WindowRect;

// DirectX 12 Objects
PDevice              g_Device;
PCommandQueue        g_CommandQueue;
PSwapChain           g_SwapChain;
PGraphicsCommandList g_CommandList;
PCommandAllocator    g_CommandAllocators[g_NumFrames];
PDescriptorHeap      g_RTVDescriptorHeap;
UINT                 g_RTVDescriptorSize;
UINT                 g_CurrentBackBufferIndex;

// Synchronization objects
PFence   g_Fence;
uint64_t g_FenceValue                    = 0;
uint64_t g_FrameFenceValues[g_NumFrames] = {};
HANDLE   g_FenceEvent;

// By default, enable V-Sync.
// Can be toggled with the V key.
bool g_VSync            = true;
bool g_TearingSupported = false;
// By default, use windowed mode.
// Can be toggled with the Alt+Enter or F11
bool g_Fullscreen = false;

void ParseCommandLineArguments()
{
    int       argc;
    wchar_t **argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

    for (size_t i = 0; i < argc; ++i)
    {
        if (wcscmp(argv[i], L"-w") == 0 || wcscmp(argv[i], L"--width") == 0)
        {
            g_ClientWidth = ::wcstol(argv[++i], nullptr, 10);
        }
        if (wcscmp(argv[i], L"-h") == 0 || wcscmp(argv[i], L"--height") == 0)
        {
            g_ClientHeight = ::wcstol(argv[++i], nullptr, 10);
        }
        if (wcscmp(argv[i], L"-warp") == 0 || wcscmp(argv[i], L"--warp") == 0)
        {
            g_UseWarp = true;
        }
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
        if (desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;
        if (FAILED(D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr)))
            continue;
        if (desc1.DedicatedVideoMemory < maxDedicatedVram)
            continue;
        maxDedicatedVram = desc1.DedicatedVideoMemory;
        Assert(adapter1.As(&adapter4));
    }
    return adapter4;
}

PDevice CreateDevice(ComPtr<IDXGIAdapter4> adapter)
{
    PDevice device;
    Assert(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device)));
#ifdef _DEBUG
    ComPtr<ID3D12InfoQueue> queue;
    if (SUCCEEDED(device.As(&queue)))
    {
        queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
    }
#endif

    // Enable debug messages in debug mode.
#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(device.As(&pInfoQueue)))
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
    return device;
}

PCommandQueue CreateCommandQueue(PDevice device, D3D12_COMMAND_LIST_TYPE type)
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type                     = type;
    desc.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask                 = 0;

    PCommandQueue queue;
    Assert(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)));
    return queue;
}

bool CheckTearingSupport()
{
    ComPtr<IDXGIFactory5> factory5;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory5))))
        return false;
    BOOL allowTearing = FALSE;
    if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
        return false;
    return allowTearing;
}

PSwapChain CreateSwapChain(HWND hWnd, PCommandQueue commandQueue, UINT width, UINT height, UINT bufferCount)
{
    UINT createFactoryFlags = 0;
#ifdef _DEBUG
    createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
    ComPtr<IDXGIFactory4> factory;
    Assert(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&factory)));

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width                 = width;
    desc.Height                = height;
    desc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Stereo                = FALSE;
    // desc.SampleDesc.Count = 1;
    // desc.SampleDesc.Quality = 0;
    desc.SampleDesc  = {1, 0};
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = bufferCount;
    desc.Scaling     = DXGI_SCALING_STRETCH;
    desc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.Flags       = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> chain1;
    Assert(factory->CreateSwapChainForHwnd(commandQueue.Get(), hWnd, &desc, nullptr, nullptr, &chain1));
    Assert(factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));
    PSwapChain chain4;
    Assert(chain1.As(&chain4));
    return chain4;
}

PDescriptorHeap CreateDescriptorHeap(PDevice device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors             = numDescriptors;
    desc.Type                       = type;

    PDescriptorHeap heap;
    Assert(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
    return heap;
}

void UpdateRenderTargetViews(PDevice device, PSwapChain swapChain, PDescriptorHeap heap)
{
    UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(heap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < g_NumFrames; ++i)
    {
        PResource backBuffer;
        Assert(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
        device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
        g_BackBuffers[i] = backBuffer;
        rtvHandle.Offset(rtvDescriptorSize);
    }
}

PCommandAllocator CreateCommandAllocator(PDevice device, D3D12_COMMAND_LIST_TYPE type)
{
    PCommandAllocator allocator;
    Assert(device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator)));
    return allocator;
}

PGraphicsCommandList CreateCommandList(PDevice device, PCommandAllocator allocator, D3D12_COMMAND_LIST_TYPE type)
{
    PGraphicsCommandList list;
    Assert(device->CreateCommandList(0, type, allocator.Get(), nullptr, IID_PPV_ARGS(&list)));
    Assert(list->Close());
    return list;
}

PFence CreateFence(PDevice device)
{
    PFence fence;
    Assert(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    return fence;
}

HANDLE CreateEventHandle()
{
    HANDLE fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!fenceEvent)
        throw std::exception("Failed to create fence event");
    return fenceEvent;
}

UINT64 Signal(PCommandQueue commandQueue, PFence fence, UINT64 &fenceValue)
{
    UINT64 fenceValueForSignal = ++fenceValue;
    Assert(commandQueue->Signal(fence.Get(), fenceValueForSignal));
    return fenceValueForSignal;
}

void WaitForFenceValue(PFence fence, UINT64 fenceValue, HANDLE fenceEvent, DWORD milliseconds = DWORD_MAX)
{
    if (fence->GetCompletedValue() < fenceValue)
    {
        Assert(fence->SetEventOnCompletion(fenceValue, fenceEvent));
        WaitForSingleObject(fenceEvent, milliseconds);
    }
}

void Flush(PCommandQueue commandQueue, PFence fence, UINT64 &fenceValue, HANDLE fenceEvent)
{
    UINT64 fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
    WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
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
    auto &&commandAllocator = g_CommandAllocators[g_CurrentBackBufferIndex];
    auto &&backBuffer       = g_BackBuffers[g_CurrentBackBufferIndex];
    commandAllocator->Reset();
    {
        g_CommandList->Reset(commandAllocator.Get(), nullptr);
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        g_CommandList->ResourceBarrier(1, &barrier);

        FLOAT                         clearColor[] = {0.4f, 0.6f, 0.9f, 1.0f};
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
            g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), g_CurrentBackBufferIndex, g_RTVDescriptorSize);
        g_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        g_CommandList->ResourceBarrier(1, &barrier);
        Assert(g_CommandList->Close());
        ID3D12CommandList *const commandLists[] = {g_CommandList.Get()};
        g_CommandQueue->ExecuteCommandLists(1, commandLists);

        UINT syncInterval = g_VSync ? 1 : 0;
        UINT presentFlags = g_TearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
        Assert(g_SwapChain->Present(syncInterval, presentFlags));
        g_FrameFenceValues[g_CurrentBackBufferIndex] = Signal(g_CommandQueue, g_Fence, g_FenceValue);

        g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
        WaitForFenceValue(g_Fence, g_FrameFenceValues[g_CurrentBackBufferIndex], g_FenceEvent);
    }
}

void Resize(UINT32 width, UINT32 height)
{
    if (g_ClientWidth == width && g_ClientHeight == height)
        return;
    g_ClientWidth  = (std::max)(1u, width);
    g_ClientHeight = (std::max)(1u, height);
    Flush(g_CommandQueue, g_Fence, g_FenceValue, g_FenceEvent);
    for (int i = 0; i < g_NumFrames; ++i)
    {
        g_BackBuffers[i].Reset();
        g_FrameFenceValues[i] = g_FrameFenceValues[g_CurrentBackBufferIndex];
    }

    DXGI_SWAP_CHAIN_DESC desc = {};
    Assert(g_SwapChain->GetDesc(&desc));
    Assert(g_SwapChain->ResizeBuffers(g_NumFrames, g_ClientWidth, g_ClientHeight, desc.BufferDesc.Format, desc.Flags));
    g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
    UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap);
}

void SetFullscreen(bool fullscreen)
{
    if (g_Fullscreen == fullscreen)
        return;
    g_Fullscreen = fullscreen;
    if (g_Fullscreen)
    {
        GetWindowRect(g_hWnd, &g_WindowRect);
        UINT windowStyle = WS_OVERLAPPED;
        SetWindowLongW(g_hWnd, GWL_STYLE, windowStyle);

        HMONITOR      hMonitor    = MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX monitorInfo = {};
        monitorInfo.cbSize        = sizeof(MONITORINFOEX);
        GetMonitorInfoW(hMonitor, &monitorInfo);

        SetWindowPos(g_hWnd,
                     HWND_TOP,
                     monitorInfo.rcMonitor.left,
                     monitorInfo.rcMonitor.top,
                     monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                     monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                     SWP_FRAMECHANGED | SWP_NOACTIVATE);
        ShowWindow(g_hWnd, SW_MAXIMIZE);
    }
    else
    {
        SetWindowLong(g_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
        SetWindowPos(g_hWnd,
                     HWND_NOTOPMOST,
                     g_WindowRect.left,
                     g_WindowRect.top,
                     g_WindowRect.right - g_WindowRect.left,
                     g_WindowRect.bottom - g_WindowRect.top,
                     SWP_FRAMECHANGED | SWP_NOACTIVATE);
        ShowWindow(g_hWnd, SW_NORMAL);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!g_IsInitialized)
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_PAINT:
        Update();
        Render();
        break;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN: {
        bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

        switch (wParam)
        {
        case 'V':
            g_VSync = !g_VSync;
            break;

        case VK_ESCAPE:
            PostQuitMessage(0);
            break;

        case VK_RETURN:
            if (alt)
                SetFullscreen(!g_Fullscreen);
            break;

        case VK_F11:
            SetFullscreen(!g_Fullscreen);
            break;
        }
        break;
    }

    case WM_SYSCHAR:
        break;

    case WM_SIZE: {
        RECT cr = {};
        GetClientRect(g_hWnd, &cr);
        LONG width  = cr.right - cr.left;
        LONG height = cr.bottom - cr.top;
        Resize(width, height);
    }

    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

int CALLBACK wWinMain(_In_ HINSTANCE     hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR        lpCmdLine,
                      _In_ int           nShowCmd)
{
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    ParseCommandLineArguments();
    EnableDebugLayer();

    g_TearingSupported = CheckTearingSupport();

    WindowClass cls(hInstance, L"DX12WindowClass", &WndProc);
    Window      mainWindow(cls, L"", g_ClientWidth, g_ClientHeight);
    g_hWnd = mainWindow.HWnd();
    GetWindowRect(g_hWnd, &g_WindowRect);

    ComPtr<IDXGIAdapter4> adapter4 = GetAdapter(g_UseWarp);
    g_Device                       = CreateDevice(adapter4);
    g_CommandQueue                 = CreateCommandQueue(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    g_SwapChain              = CreateSwapChain(g_hWnd, g_CommandQueue, g_ClientWidth, g_ClientHeight, g_NumFrames);
    g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
    g_RTVDescriptorHeap      = CreateDescriptorHeap(g_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, g_NumFrames);
    g_RTVDescriptorSize      = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap);

    for (int i = 0; i < g_NumFrames; ++i)
        g_CommandAllocators[i] = CreateCommandAllocator(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    g_CommandList =
        CreateCommandList(g_Device, g_CommandAllocators[g_CurrentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);
    g_Fence      = CreateFence(g_Device);
    g_FenceEvent = CreateEventHandle();

    g_IsInitialized = true;
    ShowWindow(g_hWnd, nShowCmd);

    MSG msg;
    while (GetMessageW(&msg, HWND_DESKTOP, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Flush(g_CommandQueue, g_Fence, g_FenceValue, g_FenceEvent);
    CloseHandle(g_FenceEvent);
    return msg.wParam;
}