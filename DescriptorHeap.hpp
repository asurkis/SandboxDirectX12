#pragma once

#include "pch.hpp"

class DescriptorHeap
{
    D3D12_DESCRIPTOR_HEAP_DESC m_Desc;
    PDescriptorHeap            m_Heap;
    UINT                       m_IncrementSize;

  public:
    DescriptorHeap();

    DescriptorHeap(PDevice                     device,
                   D3D12_DESCRIPTOR_HEAP_TYPE  type,
                   UINT                        numDescriptors,
                   D3D12_DESCRIPTOR_HEAP_FLAGS flags    = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
                   UINT                        nodeMask = 0);

    DescriptorHeap(DescriptorHeap const &)            = default;
    DescriptorHeap(DescriptorHeap &&)                 = default;
    DescriptorHeap &operator=(DescriptorHeap const &) = default;
    DescriptorHeap &operator=(DescriptorHeap &&)      = default;

    PDescriptorHeap Get() const noexcept { return m_Heap; }
    constexpr UINT  GetIncrementSize() const noexcept { return m_IncrementSize; }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUStart() const noexcept { return m_Heap->GetCPUDescriptorHandleForHeapStart(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUStart() const noexcept { return m_Heap->GetGPUDescriptorHandleForHeapStart(); }

    CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUOffset(int offset) const noexcept
    {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_Heap->GetCPUDescriptorHandleForHeapStart(), offset, m_IncrementSize);
    }

    CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUOffset(int offset) const noexcept
    {
        return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_Heap->GetGPUDescriptorHandleForHeapStart(), offset, m_IncrementSize);
    }
};
