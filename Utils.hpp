#pragma once

// #define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ios>
#include <sstream>

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
