#include "UploadBuffer.hpp"

#include "Application.hpp"
#include "Utils.hpp"

using Allocation = UploadBuffer::Allocation;

Allocation UploadBuffer::Allocate(size_t bytes, size_t alignment)
{
    if (bytes > GetPageSize())
        throw std::bad_alloc();

    if (!m_CurrentPage || !m_CurrentPage->HasSpace(bytes, alignment))
        m_CurrentPage = RequestPage();

    return m_CurrentPage->Allocate(bytes, alignment);
}

UploadBuffer::PPage UploadBuffer::RequestPage()
{
    PPage page;
    if (!m_AvailablePages.empty())
    {
        page = m_AvailablePages.front();
        m_AvailablePages.pop_front();
    }
    else
    {
        page = std::make_shared<Page>(GetPageSize());
        m_PagePool.push_back(page);
    }
    return page;
}

void UploadBuffer::Reset()
{
    m_CurrentPage    = nullptr;
    m_AvailablePages = m_PagePool;
    for (auto &&page : m_PagePool)
        page->Reset();
}

UploadBuffer::Page::Page(size_t bytes)
    : m_PageSize(bytes),
      m_Offset(0),
      m_CPU(nullptr),
      m_GPU(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
    PDevice device = Application::Get()->GetDevice();
    Assert(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                           D3D12_HEAP_FLAG_NONE,
                                           &CD3DX12_RESOURCE_DESC::Buffer(m_PageSize),
                                           D3D12_RESOURCE_STATE_GENERIC_READ,
                                           nullptr,
                                           IID_PPV_ARGS(&m_Resource)));
    m_GPU = m_Resource->GetGPUVirtualAddress();
    m_Resource->Map(0, nullptr, &m_CPU);
}

UploadBuffer::Page::~Page() { m_Resource->Unmap(0, nullptr); }

bool UploadBuffer::Page::HasSpace(size_t bytes, size_t alignment) const noexcept
{
    size_t alignedSize   = Math::AlignUp(bytes, alignment);
    size_t alignedOffset = Math::AlignUp(m_Offset, alignment);
    return alignedOffset + alignedSize <= m_PageSize;
}

UploadBuffer::Allocation UploadBuffer::Page::Allocate(size_t bytes, size_t alignment)
{
    // if (!HasSpace(bytes, alignment))
    //     throw std::bad_alloc();

    size_t alignedSize = Math::AlignUp(bytes, alignment);
    m_Offset           = Math::AlignUp(m_Offset, alignment);

    Allocation allocation;
    allocation.cpu = static_cast<char *>(m_CPU) + m_Offset;
    allocation.gpu = m_GPU + m_Offset;

    m_Offset += alignedSize;
    return allocation;
}
