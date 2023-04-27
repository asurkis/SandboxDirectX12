#pragma once

#include "pch.hpp"

class UploadBuffer
{
  public:
    struct Allocation
    {
        void                     *cpu;
        D3D12_GPU_VIRTUAL_ADDRESS gpu;
    };

    explicit UploadBuffer(size_t pageSize = 2 << 20)
        : m_PageSize(pageSize)
    {
    }

    size_t GetPageSize() const noexcept { return m_PageSize; }

    Allocation Allocate(size_t bytes, size_t alignment);
    void       Reset();

  private:
    class Page
    {
        PResource                 m_Resource;
        void                     *m_CPU;
        D3D12_GPU_VIRTUAL_ADDRESS m_GPU;
        size_t                    m_PageSize;
        size_t                    m_Offset;

      public:
        Page(size_t bytes);
        ~Page();
        bool       HasSpace(size_t bytes, size_t alignment) const noexcept;
        Allocation Allocate(size_t bytes, size_t alignment);
        void       Reset() noexcept { m_Offset = 0; }
    };

    using PPage    = std::shared_ptr<Page>;
    using PagePool = std::deque<PPage>;

    PPage RequestPage();

    PagePool m_PagePool;
    PagePool m_AvailablePages;
    PPage    m_CurrentPage;
    size_t   m_PageSize;
};
