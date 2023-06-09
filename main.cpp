#ifndef _DEBUG
#define _DEBUG
#endif

// #define WIN32_LEAN_AND_MEAN
#include "Application.hpp"

int CALLBACK wWinMain(_In_ HINSTANCE     hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR        lpCmdLine,
                      _In_ int           nShowCmd)
{
    try
    {
        Assert(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        Application application(hInstance);
        return application.Run(nShowCmd);
    }
    catch (const std::exception &e)
    {
        OutputDebugStringA(e.what());
        return -1;
    }
}
