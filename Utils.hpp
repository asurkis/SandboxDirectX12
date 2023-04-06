#pragma once

// #define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ios>
#include <sstream>
#include <stdexcept>

#define Assert(hr)                                                                                                     \
    do                                                                                                                 \
    {                                                                                                                  \
        HRESULT hr_ = hr;                                                                                              \
        if (FAILED(hr_))                                                                                               \
        {                                                                                                              \
            std::stringstream ss_;                                                                                     \
            ss_ << __FILE__ ":" #hr " HRESULT = 0x" << std::hex << hr_;                                                \
            throw std::runtime_error(ss_.str());                                                                       \
        }                                                                                                              \
    } while (0)

// #define WIN32_LEAN_AND_MEAN
#include <Windows.h> // For HRESULT

// From DXSampleHelper.h
// Source: https://github.com/Microsoft/DirectX-Graphics-Samples
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
        throw std::exception();
}