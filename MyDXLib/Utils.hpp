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

inline constexpr size_t BACK_BUFFER_COUNT      = 3;
inline constexpr size_t INTERMEDIATE_RTV_COUNT = 1;

inline constexpr size_t BACK_BUFFER_START      = 0;
inline constexpr size_t INTERMEDIATE_RTV_START = BACK_BUFFER_START + BACK_BUFFER_COUNT;
inline constexpr size_t RTV_COUNT              = INTERMEDIATE_RTV_START + INTERMEDIATE_RTV_COUNT;

PResource QueryUploadBuffer(PDevice device, ResourceUploadBatch &rub, const void *data, size_t bufSize);
