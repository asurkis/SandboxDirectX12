#include "Utils.hpp"

std::pair<PResource, PResource> QueryUploadBuffer(PGraphicsCommandList commandList, const void *data, size_t bufSize)
{
    PDevice   device;
    PResource buffer;
    PResource intermediate;

    Assert(commandList->GetDevice(IID_PPV_ARGS(&device)));

    Assert(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                           D3D12_HEAP_FLAG_NONE,
                                           &CD3DX12_RESOURCE_DESC::Buffer(bufSize),
                                           D3D12_RESOURCE_STATE_COMMON,
                                           nullptr,
                                           IID_PPV_ARGS(&buffer)));
    Assert(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                           D3D12_HEAP_FLAG_NONE,
                                           &CD3DX12_RESOURCE_DESC::Buffer(bufSize),
                                           D3D12_RESOURCE_STATE_GENERIC_READ,
                                           nullptr,
                                           IID_PPV_ARGS(&intermediate)));

    D3D12_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pData                  = data;
    subresourceData.RowPitch               = bufSize;
    subresourceData.SlicePitch             = bufSize;
    UpdateSubresources(commandList.Get(), buffer.Get(), intermediate.Get(), 0, 0, 1, &subresourceData);

    return std::make_pair(buffer, intermediate);
}
