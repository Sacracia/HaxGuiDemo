#include "hax.h"

#include <malloc.h>

namespace Hax
{
    struct AllocHeader
    {
        size_t Size;
        size_t Magic;
    };

    static_assert(sizeof(AllocHeader) % alignof(std::max_align_t) == 0);

    Allocator g_GlobalAlloc{"Global"};
    Allocator g_StateAlloc{"States"};

    void* Allocator::Alloc(size_t size)
    {
        void* ptr = malloc(size + sizeof(AllocHeader));
        if (!ptr) return nullptr;

        m_TotalAllocated += size;
        m_MaxAllocated = Max(m_MaxAllocated, m_TotalAllocated);

        AllocHeader* header = (AllocHeader*)ptr;
        header->Size = size;
        header->Magic = (size_t)this;

        return (void*)((uint8*)ptr + sizeof(AllocHeader));
    }

    void Allocator::Free(void* ptr)
    {
        if (!ptr) return;

        AllocHeader* header = (AllocHeader*)((uint8*)ptr - sizeof(AllocHeader));
        HAX_ASSERT(header->Magic == (size_t)this);

        header->Magic = 0;
        m_TotalAllocated -= header->Size;

        free(header);
    }
}