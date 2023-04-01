// #define WIN32_LEAN_AND_MEAN
#include "MainWindow.hpp"
#include "Utils.hpp"
#include <Windows.h>

#if false
#include <DirectXMath.h>
#include <algorithm>
#include <cstdint>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl.h>
// #include <d3dx12.h>

using Microsoft::WRL::ComPtr;

const uint8_t g_NumFrames = 3;
bool g_UseWarp = false;
uint32_t g_ClientWidth = 1280;
uint32_t g_ClientHeight = 720;

bool g_IsInitialized = false;

// Window handle.
HWND g_hWnd;
// Window rectangle (used to toggle fullscreen state).
RECT g_WindowRect;

// DirectX 12 Objects
ComPtr<ID3D12Device2> g_Device;
ComPtr<ID3D12CommandQueue> g_CommandQueue;
ComPtr<IDXGISwapChain4> g_SwapChain;
ComPtr<ID3D12Resource> g_BackBuffers[g_NumFrames];
ComPtr<ID3D12GraphicsCommandList> g_CommandList;
ComPtr<ID3D12CommandAllocator> g_CommandAllocators[g_NumFrames];
ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
UINT g_RTVDescriptorSize;
UINT g_CurrentBackBufferIndex;

// Synchronization objects
ComPtr<ID3D12Fence> g_Fence;
uint64_t g_FenceValue = 0;
uint64_t g_FrameFenceValues[g_NumFrames] = {};
HANDLE g_FenceEvent;

// By default, enable V-Sync.
// Can be toggled with the V key.
bool g_VSync = true;
bool g_TearingSupported = false;
// By default, use windowed mode.
// Can be toggled with the Alt+Enter or F11
bool g_Fullscreen = false;

void ParseCommandLineArguments()
{
    int argc;
    wchar_t **argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

    for (size_t i = 0; i < argc; ++i)
    {
        if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"--width") == 0)
        {
            g_ClientWidth = ::wcstol(argv[++i], nullptr, 10);
        }
        if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0)
        {
            g_ClientHeight = ::wcstol(argv[++i], nullptr, 10);
        }
        if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0)
        {
            g_UseWarp = true;
        }
    }

    // Free memory allocated by CommandLineToArgvW
    ::LocalFree(argv);
}

void EnableDebugLayer()
{
#if defined(_DEBUG)
    // Always enable the debug layer before doing anything DX12 related
    // so all possible errors generated while creating DX12 objects
    // are caught by the debug layer.
    ComPtr<ID3D12Debug> debugInterface;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
#endif
}
#endif

// Window callback function.
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
    WindowClass cls(hInstance, L"DX12WindowClass", &WndProc);
    Window mainWindow(cls, L"", 1280, 720);
    ShowWindow(mainWindow.HWnd(), nShowCmd);
    MSG msg;
    while (GetMessageW(&msg, HWND_DESKTOP, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}