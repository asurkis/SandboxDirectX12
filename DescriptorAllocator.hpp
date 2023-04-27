#pragma once

#include "pch.hpp"

class DescriptorAllocator
{
  public:
    DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT32 numDescriptorsPerHeap = 256)
        : m_HeapType(type),
          m_NumDescriptorsPerHeap(numDescriptorsPerHeap)
    {
    }

    virtual ~DescriptorAllocator();

    class Allocation;
    class Page;

    Allocation Allocate(UINT32 numDescriptors = 1);
    void       ReleaseStaleDescriptors(UINT64 frameNumber);

  private:
    using PPage    = std::shared_ptr<Page>;
    using HeapPool = std::vector<PPage>;

    PPage CreateAllocatorPage();

    D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;
    UINT32                     m_NumDescriptorsPerHeap;
    HeapPool                   m_HeapPool;
    std::unordered_set<size_t> m_AvailableHeaps;
    std::mutex                 m_Mutex;
};

class DescriptorAllocator::Page : public std::enable_shared_from_this<DescriptorAllocator::Page>
{
    using OffsetType = UINT32;
    using SizeType   = UINT32;

    struct FreeBlockInfo;
    using FreeListByOffset = std::map<OffsetType, FreeBlockInfo>;
    using FreeListBySize   = std::multimap<SizeType, FreeListByOffset::iterator>;

    struct FreeBlockInfo
    {
        SizeType                 Size;
        FreeListBySize::iterator FreeListBySizeIt;

        FreeBlockInfo(SizeType size)
            : Size(size)
        {
        }
    };

    struct StaleDescriptorInfo
    {
        OffsetType Offset;
        SizeType   Size;
        UINT64     Frame;

        StaleDescriptorInfo(OffsetType offset, SizeType size, UINT64 frame) noexcept
            : Offset(offset),
              Size(size),
              Frame(frame)
        {
        }
    };

    using StaleDescriptorQueue = std::queue<StaleDescriptorInfo>;

    FreeListByOffset     m_FreeListByOffset;
    FreeListBySize       m_FreeListBySize;
    StaleDescriptorQueue m_StaleDescriptorQueue;

    PDescriptorHeap               m_DescriptorHeap;
    D3D12_DESCRIPTOR_HEAP_TYPE    m_HeapType;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_BaseDescriptor;
    UINT32                        m_DescriptorHandleIncrementSize;
    UINT32                        m_NumDescriptorsInHeap;
    UINT32                        m_NumFreeHandles;

    std::mutex m_Mutex;

  protected:
    UINT32 ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle) const noexcept
    {
        return static_cast<UINT32>(handle.ptr - m_BaseDescriptor.ptr) / m_DescriptorHandleIncrementSize;
    }

    void AddNewBlock(UINT32 offset, UINT32 numDescriptors);
    void FreeBlock(UINT32 offset, UINT32 numDescriptors);

  public:
    Page(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT32 numDescriptors);
    DescriptorAllocator::Allocation Allocate(UINT32 numDescriptors);
    void                            Free(Allocation &&allocation, UINT64 frameNumber);
    void                            ReleaseStaleDescriptors(UINT64 frameNumber);

    D3D12_DESCRIPTOR_HEAP_TYPE GetType() const noexcept { return m_HeapType; }
    UINT32                     NumFreeHandles() const noexcept { return m_NumFreeHandles; }
    bool                       HasSpace(UINT32 numDescriptors) const noexcept
    {
        return m_FreeListBySize.lower_bound(numDescriptors) != m_FreeListBySize.end();
    }
};

class DescriptorAllocator::Allocation
{
    D3D12_CPU_DESCRIPTOR_HANDLE m_Descriptor;
    UINT32                      m_NumHandles;
    UINT32                      m_DescriptorSize;
    PPage                       m_Page;

    void Free();

  public:
    explicit Allocation() noexcept
        : Allocation({0}, 0, 0, nullptr)
    {
    }

    explicit Allocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor,
                        UINT32                      numHandles,
                        UINT32                      descriptorSize,
                        PPage                       page) noexcept
        : m_Descriptor(descriptor),
          m_NumHandles(numHandles),
          m_DescriptorSize(descriptorSize),
          m_Page(std::move(page))
    {
    }

    ~Allocation() { Free(); }

    Allocation(const Allocation &)            = delete;
    Allocation &operator=(const Allocation &) = delete;

    Allocation(Allocation &&that)
        : m_Descriptor(that.m_Descriptor),
          m_NumHandles(that.m_NumHandles),
          m_DescriptorSize(that.m_DescriptorSize),
          m_Page(std::move(that.m_Page))
    {
        that.m_Descriptor.ptr = 0;
        that.m_NumHandles     = 0;
        that.m_DescriptorSize = 0;
    }

    Allocation &operator=(Allocation &&that)
    {
        m_Descriptor     = that.m_Descriptor;
        m_NumHandles     = that.m_NumHandles;
        m_DescriptorSize = that.m_DescriptorSize;
        m_Page           = std::move(that.m_Page);

        that.m_Descriptor.ptr = 0;
        that.m_NumHandles     = 0;
        that.m_DescriptorSize = 0;

        return *this;
    }

    bool IsNull() const noexcept { return m_Descriptor.ptr == 0; }

    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(UINT32 offset = 0) const
    {
        if (offset >= m_NumHandles)
            throw std::exception();
        return {m_Descriptor.ptr + m_DescriptorSize * offset};
    }

    UINT32 GetNumHandles() const noexcept { return m_NumHandles; }
    PPage  GetDescriptorAllocatorPage() const noexcept { return m_Page; }
};