#pragma once

#include "Commons.hpp"

class Device
{
    PDevice m_Device;

  public:
    Device(ComPtr<IDXGIAdapter4> adapter)
    {
        Assert(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_Device)));
#ifdef _DEBUG
        ComPtr<ID3D12InfoQueue> queue;
        if (SUCCEEDED(m_Device.As(&queue)))
        {
            queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        }
#endif

        // Enable debug messages in debug mode.
#if defined(_DEBUG)
        ComPtr<ID3D12InfoQueue> pInfoQueue;
        if (SUCCEEDED(m_Device.As(&pInfoQueue)))
        {
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);

            // Suppress whole categories of messages
            // D3D12_MESSAGE_CATEGORY Categories[] = {};

            // Suppress messages based on their severity level
            D3D12_MESSAGE_SEVERITY Severities[] = {D3D12_MESSAGE_SEVERITY_INFO};

            // Suppress individual messages by their ID
            D3D12_MESSAGE_ID DenyIds[] = {
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE, // I'm really not sure how to avoid this
                                                                              // message.
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,   // This warning occurs when using capture frame while graphics
                                                          // debugging.
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE, // This warning occurs when using capture frame while graphics
                                                          // debugging.

                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                // Workarounds for debug layer issues on hybrid-graphics systems
                D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
                D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
            };

            D3D12_INFO_QUEUE_FILTER NewFilter = {};
            // NewFilter.DenyList.NumCategories = _countof(Categories);
            // NewFilter.DenyList.pCategoryList = Categories;
            NewFilter.DenyList.NumSeverities = _countof(Severities);
            NewFilter.DenyList.pSeverityList = Severities;
            NewFilter.DenyList.NumIDs        = _countof(DenyIds);
            NewFilter.DenyList.pIDList       = DenyIds;

            Assert(pInfoQueue->PushStorageFilter(&NewFilter));
        }
#endif
    }

    PDevice Get() const noexcept { return m_Device; }

    PDescriptorHeap CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors             = numDescriptors;
        desc.Type                       = type;

        PDescriptorHeap heap;
        Assert(m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
        return heap;
    }

    PCommandAllocator CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type)
    {
        PCommandAllocator allocator;
        Assert(m_Device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator)));
        return allocator;
    }

    PGraphicsCommandList CreateCommandList(PCommandAllocator allocator, D3D12_COMMAND_LIST_TYPE type)
    {
        PGraphicsCommandList list;
        Assert(m_Device->CreateCommandList(0, type, allocator.Get(), nullptr, IID_PPV_ARGS(&list)));
        // Assert(list->Close());
        return list;
    }
};
