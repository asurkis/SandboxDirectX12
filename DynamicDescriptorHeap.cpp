#include "DynamicDescriptorHeap.hpp"

#include "Application.hpp"
#include "RootSignature.hpp"

DynamicDescriptorHeap::DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT32 numDescriptorsPerHeap)
    : m_DescriptorHeapType(type),
      m_NumDescriptorsPerHeap(numDescriptorsPerHeap)
{
    PDevice device                  = Application::Get()->GetDevice();
    m_DescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(type);
    m_DescriptorHandleCache         = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(m_NumDescriptorsPerHeap);
}

void DynamicDescriptorHeap::ParseRootSignature(const RootSignature &rootSignature)
{
    m_StaleDescriptorTableBitMask = 0;
    auto &&desc                   = rootSignature.GetRootSignatureDesc();
    m_DescriptorTableBitMask      = rootSignature.GetDescriptorTableBitMask(m_DescriptorHeapType);
    UINT32 descriptorTableBitMask = m_DescriptorTableBitMask;

    UINT32 currentOffset = 0;
    DWORD  rootIndex;
    while (_BitScanForward(&rootIndex, descriptorTableBitMask) && rootIndex < desc.NumParameters)
    {
        UINT32 numDescriptors = rootSignature.GetNumDescriptors(rootIndex);

        DescriptorTableCache &cache = m_DescriptorTableCache[rootIndex];
        cache.NumDescriptors        = numDescriptors;
        cache.BaseDescriptor        = &m_DescriptorHandleCache[currentOffset];
        currentOffset += numDescriptors;
        descriptorTableBitMask ^= 1 << rootIndex;
    }

    if (currentOffset > m_NumDescriptorsPerHeap)
        throw std::exception("The root signature requires more than the maximum number of descriptors per descriptor "
                             "heap. Consider increasing the maximum number of descriptors per descriptor heap.");
}

void DynamicDescriptorHeap::StageDescriptors(UINT32                      rootParameterIndex,
                                             UINT32                      offset,
                                             UINT32                      numDescriptors,
                                             D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor)
{
    if (numDescriptors > m_NumDescriptorsPerHeap || rootParameterIndex >= MaxDescriptorTables)
        throw std::bad_alloc();

    DescriptorTableCache &cache = m_DescriptorTableCache[rootParameterIndex];
    if (offset + numDescriptors > cache.NumDescriptors)
        throw std::length_error("Number of descriptors exceeds the number of descriptors in the descriptor table.");

    D3D12_CPU_DESCRIPTOR_HANDLE *dstDescriptor = cache.BaseDescriptor + offset;
    for (uint32_t i = 0; i < numDescriptors; ++i)
        dstDescriptor[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(srcDescriptor, i, m_DescriptorHandleIncrementSize);

    m_StaleDescriptorTableBitMask |= 1 << rootParameterIndex;
}

uint32_t DynamicDescriptorHeap::ComputeStaleDescriptorCount() const
{
    uint32_t numStaleDescriptors = 0;
    DWORD    i;
    DWORD    staleDescriptorsBitMask = m_StaleDescriptorTableBitMask;

    while (_BitScanForward(&i, staleDescriptorsBitMask))
    {
        numStaleDescriptors += m_DescriptorTableCache[i].NumDescriptors;
        staleDescriptorsBitMask ^= (1 << i);
    }

    return numStaleDescriptors;
}
