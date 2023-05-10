#include "Utils.hpp"

PResource QueryUploadBuffer(PDevice device, ResourceUploadBatch &rub, const void *data, size_t bufSize)
{
    PResource buffer;
    PResource intermediate;

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
    rub.Upload(buffer.Get(), 0, &subresourceData, 1);

    return buffer;
}
