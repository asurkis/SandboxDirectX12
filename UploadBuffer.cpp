#include "UploadBuffer.hpp"

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
    for (auto page : m_PagePool)
        page->Reset();
}