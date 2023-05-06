#pragma once

#include "pch.hpp"

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

struct Math
{
    Math() = delete;

    static size_t AlignUp(size_t bytes, size_t alignment)
    {
        size_t tmp = (bytes + alignment - 1);
        return tmp - tmp % alignment;
    }
};

static inline constexpr size_t BUFFER_COUNT = 3;
