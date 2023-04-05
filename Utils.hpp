#pragma once

// #define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <exception>

#define Assert(hr)                                                                                                     \
    do                                                                                                                 \
    {                                                                                                                  \
        if (FAILED(hr))                                                                                                \
            throw std::exception(__FILE__ ":" #hr);                                                                    \
    } while (0)
