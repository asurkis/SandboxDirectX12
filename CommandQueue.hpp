#pragma once

#include "Commons.hpp"
#include "Utils.hpp"

class CommandQueue
{
    PDevice                 m_Device;
    D3D12_COMMAND_LIST_TYPE m_CommandListType;
    PCommandQueue           m_CommandQueue;
    PFence                  m_Fence;
    HANDLE                  m_FenceEvent;

  public:
    CommandQueue(PDevice device, D3D12_COMMAND_LIST_TYPE type) : m_Device(device), m_CommandListType(type)
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type                     = type;
        desc.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask                 = 0;

        Assert(m_Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_CommandQueue)));
        Assert(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));

        m_FenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!m_FenceEvent)
            throw std::exception("Failed to create fence event");
    }

    ~CommandQueue()
    {
        CloseHandle(m_FenceEvent);
    }

    PCommandQueue Get() const noexcept
    {
        return m_CommandQueue;
    }

    void ExecuteCommandLists(UINT nCommandLists, ID3D12CommandList *const *commandLists)
    {
        m_CommandQueue->ExecuteCommandLists(1, commandLists);
    }

    UINT64 Signal(UINT64 &fenceValue)
    {
        UINT64 fenceValueForSignal = ++fenceValue;
        Assert(m_CommandQueue->Signal(m_Fence.Get(), fenceValueForSignal));
        return fenceValueForSignal;
    }

    void WaitForFenceValue(UINT64 fenceValue, DWORD milliseconds = DWORD_MAX)
    {
        if (m_Fence->GetCompletedValue() < fenceValue)
        {
            Assert(m_Fence->SetEventOnCompletion(fenceValue, m_FenceEvent));
            WaitForSingleObject(m_FenceEvent, milliseconds);
        }
    }

    void Flush(UINT64 &fenceValue)
    {
        UINT64 fenceValueForSignal = Signal(fenceValue);
        WaitForFenceValue(fenceValueForSignal);
    }
};
