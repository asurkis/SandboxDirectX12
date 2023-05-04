#pragma once

#include "pch.hpp"

#include "Utils.hpp"

class CommandQueue
{
    struct CommandAllocatorEntry
    {
        UINT64            FenceValue;
        PCommandAllocator CommandAllocator;
    };

    using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
    using CommandListQueue      = std::queue<PGraphicsCommandList>;

    PDevice                 m_Device;
    D3D12_COMMAND_LIST_TYPE m_CommandListType;
    PCommandAllocator       m_Allocator;
    PGraphicsCommandList    m_CommandList;
    PCommandQueue           m_CommandQueue;
    PFence                  m_Fence;
    UINT64                  m_FenceValue = 0;
    HANDLE                  m_FenceEvent = nullptr;

    CommandAllocatorQueue m_CommandAllocatorQueue;
    CommandListQueue      m_CommandListQueue;

  public:
    CommandQueue(PDevice device, D3D12_COMMAND_LIST_TYPE type)
        : m_Device(device),
          m_CommandListType(type)
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

    ~CommandQueue() { CloseHandle(m_FenceEvent); }

    PCommandQueue Get() const noexcept { return m_CommandQueue; }

    PCommandAllocator CreateCommandAllocator()
    {
        PCommandAllocator allocator;
        Assert(m_Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&allocator)));
        return allocator;
    }

    PGraphicsCommandList CreateCommandList(PCommandAllocator allocator)
    {
        PGraphicsCommandList list;
        Assert(m_Device->CreateCommandList(0, m_CommandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&list)));
        return list;
    }

    PSwapChain CreateSwapChain(
        PFactory factory, HWND hWnd, bool tearingSupport, UINT width, UINT height, UINT bufferCount)
    {
        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width                 = width;
        desc.Height                = height;
        desc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.Stereo                = FALSE;
        // desc.SampleDesc.Count = 1;
        // desc.SampleDesc.Quality = 0;
        desc.SampleDesc  = {1, 0};
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = bufferCount;
        desc.Scaling     = DXGI_SCALING_STRETCH;
        desc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
        desc.Flags       = tearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        ComPtr<IDXGISwapChain1> chain1;
        Assert(factory->CreateSwapChainForHwnd(m_CommandQueue.Get(), hWnd, &desc, nullptr, nullptr, &chain1));
        Assert(factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));
        PSwapChain chain4;
        Assert(chain1.As(&chain4));
        return chain4;
    }

    PGraphicsCommandList GetCommandList()
    {
        PCommandAllocator allocator;
        if (!m_CommandAllocatorQueue.empty() && IsFenceComplete(m_CommandAllocatorQueue.front().FenceValue))
        {
            allocator = m_CommandAllocatorQueue.front().CommandAllocator;
            m_CommandAllocatorQueue.pop();
            Assert(allocator->Reset());
        }
        else
        {
            allocator = CreateCommandAllocator();
        }

        PGraphicsCommandList commandList;
        if (!m_CommandListQueue.empty())
        {
            commandList = m_CommandListQueue.front();
            m_CommandListQueue.pop();
            Assert(commandList->Reset(allocator.Get(), nullptr));
        }
        else
        {
            commandList = CreateCommandList(allocator);
        }

        Assert(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), allocator.Get()));
        return commandList;
    }

    UINT64 ExecuteCommandList(PGraphicsCommandList commandList)
    {
        Assert(commandList->Close());

        // ID3D12CommandAllocator *allocator = nullptr;
        // UINT                    dataSize  = sizeof(allocator);
        // Assert(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &allocator));

        ID3D12CommandList *const commandLists[] = {commandList.Get()};
        m_CommandQueue->ExecuteCommandLists(1, commandLists);

        UINT64 fenceValue = Signal();
        // m_CommandAllocatorQueue.push(CommandAllocatorEntry {fenceValue, allocator});
        // m_CommandListQueue.push(commandList);
        // allocator->Release();
        return fenceValue;
    }

    UINT64 Signal()
    {
        UINT64 fenceValueForSignal = ++m_FenceValue;
        Assert(m_CommandQueue->Signal(m_Fence.Get(), fenceValueForSignal));
        return fenceValueForSignal;
    }

    bool IsFenceComplete(UINT64 fenceValue) { return m_Fence->GetCompletedValue() >= fenceValue; }

    void WaitForFenceValue(UINT64 fenceValue, DWORD milliseconds = DWORD_MAX)
    {
        if (IsFenceComplete(fenceValue))
            return;
        Assert(m_Fence->SetEventOnCompletion(fenceValue, m_FenceEvent));
        WaitForSingleObject(m_FenceEvent, milliseconds);
    }

    void Flush()
    {
        UINT64 fenceValueForSignal = Signal();
        WaitForFenceValue(fenceValueForSignal);
    }
};
