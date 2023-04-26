#pragma once

#include "pch.hpp"

class Device
{
    PDevice m_Device;

  public:
    Device(ComPtr<IDXGIAdapter4> adapter);
    PDevice Get() const noexcept { return m_Device; }

    PDescriptorHeap CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors);
};
