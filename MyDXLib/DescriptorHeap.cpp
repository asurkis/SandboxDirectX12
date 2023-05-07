#include "DescriptorHeap.hpp"
#include "Utils.hpp"

DescriptorHeap::DescriptorHeap()
    : m_Desc{},
      m_Heap(nullptr),
      m_IncrementSize(0)
{
}

DescriptorHeap::DescriptorHeap(PDevice                     device,
                               D3D12_DESCRIPTOR_HEAP_TYPE  type,
                               UINT                        numDescriptors,
                               D3D12_DESCRIPTOR_HEAP_FLAGS flags,
                               UINT                        nodeMask)
    : m_IncrementSize(device->GetDescriptorHandleIncrementSize(type))
{
    m_Desc.Type           = type;
    m_Desc.NumDescriptors = numDescriptors;
    m_Desc.Flags          = flags;
    m_Desc.NodeMask       = nodeMask;
    Assert(device->CreateDescriptorHeap(&m_Desc, IID_PPV_ARGS(&m_Heap)));
}
