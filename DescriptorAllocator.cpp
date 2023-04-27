#include "DescriptorAllocator.hpp"

#include "Application.hpp"

DescriptorAllocator::PPage DescriptorAllocator::CreateAllocatorPage()
{
    auto newPage = std::make_shared<Page>(m_HeapType, m_NumDescriptorsPerHeap);
    m_AvailableHeaps.insert(m_HeapPool.size());
    m_HeapPool.emplace_back(newPage);
    return newPage;
}

DescriptorAllocator::Allocation DescriptorAllocator::Allocate(UINT32 numDescriptors)
{
    std::lock_guard lock{m_Mutex};
    Allocation      allocation;

    for (auto iter = m_AvailableHeaps.begin(); iter != m_AvailableHeaps.end(); ++iter)
    {
        PPage page = m_HeapPool[*iter];
        allocation = page->Allocate(numDescriptors);
        if (page->NumFreeHandles() == 0)
            iter = m_AvailableHeaps.erase(iter);
        if (!allocation.IsNull())
            break;
    }

    if (allocation.IsNull())
    {
        m_NumDescriptorsPerHeap = (std::max)(m_NumDescriptorsPerHeap, numDescriptors);
        auto newPage            = CreateAllocatorPage();
        allocation              = newPage->Allocate(numDescriptors);
    }

    return allocation;
}

void DescriptorAllocator::ReleaseStaleDescriptors(UINT64 frameNumber)
{
    std::lock_guard lock{m_Mutex};
    for (size_t i = 0; i < m_HeapPool.size(); ++i)
    {
        PPage page = m_HeapPool[i];
        page->ReleaseStaleDescriptors(frameNumber);
        if (page->NumFreeHandles() > 0)
            m_AvailableHeaps.insert(i);
    }
}

DescriptorAllocator::Page::Page(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT32 numDescriptors)
    : m_HeapType(type),
      m_NumDescriptorsInHeap(numDescriptors)
{
    PDevice                    device = Application::Get()->GetDevice();
    D3D12_DESCRIPTOR_HEAP_DESC desc   = {};
    desc.Type                         = m_HeapType;
    desc.NumDescriptors               = numDescriptors;
    Assert(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_DescriptorHeap)));
    m_BaseDescriptor                = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    m_DescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(m_HeapType);
    m_NumFreeHandles                = m_NumDescriptorsInHeap;
    AddNewBlock(0, m_NumFreeHandles);
}

void DescriptorAllocator::Page::AddNewBlock(UINT32 offset, UINT32 numDescriptors)
{
    auto offsetIt                     = m_FreeListByOffset.emplace(offset, numDescriptors).first;
    auto sizeIt                       = m_FreeListBySize.emplace(numDescriptors, offsetIt);
    offsetIt->second.FreeListBySizeIt = sizeIt;
}

DescriptorAllocator::Allocation DescriptorAllocator::Page::Allocate(UINT32 numDescriptors)
{
    std::lock_guard lock{m_Mutex};
    if (numDescriptors > m_NumFreeHandles)
        return Allocation();

    auto smallestBlockIt = m_FreeListBySize.lower_bound(numDescriptors);
    if (smallestBlockIt == m_FreeListBySize.end())
        return Allocation();

    auto [blockSize, offsetIt] = *smallestBlockIt;
    auto offset                = offsetIt->first;

    m_FreeListBySize.erase(smallestBlockIt);
    m_FreeListByOffset.erase(offsetIt);

    auto newOffset = offset + numDescriptors;
    auto newSize   = blockSize - numDescriptors;
    if (newSize > 0)
        AddNewBlock(newOffset, newSize);

    m_NumFreeHandles -= numDescriptors;

    return Allocation(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_BaseDescriptor, offset, m_DescriptorHandleIncrementSize),
                      numDescriptors,
                      m_DescriptorHandleIncrementSize,
                      shared_from_this());
}

void DescriptorAllocator::Page::Free(DescriptorAllocator::Allocation &&descriptor, UINT64 frameNumber)
{
    auto            offset = ComputeOffset(descriptor.GetDescriptorHandle());
    std::lock_guard lock{m_Mutex};
    m_StaleDescriptorQueue.emplace(offset, descriptor.GetNumHandles(), frameNumber);
}

void DescriptorAllocator::Page::FreeBlock(UINT32 offset, UINT32 numDescriptors)
{
    auto nextBlockIt = m_FreeListByOffset.upper_bound(offset);
    auto prevBlockIt = nextBlockIt;
    if (prevBlockIt != m_FreeListByOffset.begin())
        --prevBlockIt;
    else
        prevBlockIt = m_FreeListByOffset.end();

    m_NumFreeHandles += numDescriptors;

    if (nextBlockIt != m_FreeListByOffset.end() && offset + numDescriptors == nextBlockIt->first)
    {
        numDescriptors += nextBlockIt->second.Size;
        m_FreeListBySize.erase(nextBlockIt->second.FreeListBySizeIt);
        m_FreeListByOffset.erase(nextBlockIt);
    }

    if (prevBlockIt != m_FreeListByOffset.end() && offset == prevBlockIt->first + prevBlockIt->second.Size)
    {
        offset = prevBlockIt->first;
        numDescriptors += prevBlockIt->second.Size;
        m_FreeListBySize.erase(prevBlockIt->second.FreeListBySizeIt);
        m_FreeListByOffset.erase(prevBlockIt);
    }

    AddNewBlock(offset, numDescriptors);
}

void DescriptorAllocator::Page::ReleaseStaleDescriptors(UINT64 frameNumber)
{
    std::lock_guard lock{m_Mutex};
    while (!m_StaleDescriptorQueue.empty())
    {
        auto &staleDescriptor = m_StaleDescriptorQueue.front();
        if (staleDescriptor.Frame > frameNumber)
            break;
        auto offset         = staleDescriptor.Offset;
        auto numDescriptors = staleDescriptor.Size;
        FreeBlock(offset, numDescriptors);
        m_StaleDescriptorQueue.pop();
    }
}

void DescriptorAllocator::Allocation::Free()
{
    if (IsNull() || !m_Page)
        return;
    m_Page->Free(std::move(*this), Application::Get()->GetFrameCount());
    m_Descriptor.ptr = 0;
    m_NumHandles     = 0;
    m_DescriptorSize = 0;
    m_Page.reset();
}