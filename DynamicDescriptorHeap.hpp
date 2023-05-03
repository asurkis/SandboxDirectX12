#pragma once

#include "pch.hpp"

class CommandList;
class RootSignature;

class DynamicDescriptorHeap
{
    PDescriptorHeap RequestDescriptorHeap();
    PDescriptorHeap CreateDescriptorHeap();
    UINT32          ComputeStaleDescriptorCount() const;

    static const UINT32 MaxDescriptorTables = 32;

    struct DescriptorTableCache
    {
        UINT32                       NumDescriptors = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE *BaseDescriptor = nullptr;

        void Reset()
        {
            NumDescriptors = 0;
            BaseDescriptor = nullptr;
        }
    };

    D3D12_DESCRIPTOR_HEAP_TYPE m_DescriptorHeapType;
    UINT32                     m_NumDescriptorsPerHeap;
    UINT32                     m_DescriptorHandleIncrementSize;

    std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> m_DescriptorHandleCache;
    DescriptorTableCache                           m_DescriptorTableCache[MaxDescriptorTables];

    UINT32 m_DescriptorTableBitMask = 0;
    UINT32 m_StaleDescriptorTableBitMask = 0;

    using DescriptorHeapPool = std::queue<PDescriptorHeap>;
    DescriptorHeapPool m_DescriptorHeapPool;
    DescriptorHeapPool m_AvailableDescriptorHeaps;

    PDescriptorHeap               m_CurrentDescriptorHeap;
    CD3DX12_GPU_DESCRIPTOR_HANDLE m_CurrentGpuDescriptorHandle = D3D12_DEFAULT;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_CurrentCpuDescriptorHandle = D3D12_DEFAULT;

    UINT32 m_NumFreeHandles = 0;

  public:
    DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT32 numDescriptorsPerHeap = 1024);
    virtual ~DynamicDescriptorHeap();

    void StageDescriptors(UINT32                      rootParameterIndex,
                          UINT32                      offset,
                          UINT32                      numDescriptors,
                          D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptors);

    void CommitStagedDescriptors(CommandList &commandList, std::function<void()> setFunc);
    void CommitStagedDescriptorsForDraw(CommandList &commandList);
    void CommitStagedDescriptorsForDispatch(CommandList &commandList);

    D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptor(CommandList &commandList, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor);

    void ParseRootSignature(const RootSignature &rootSignature);
    void Reset();
};