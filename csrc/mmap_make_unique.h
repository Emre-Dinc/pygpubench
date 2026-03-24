// Copyright (c) 2026 Erik Schultheis
// SPDX-License-Identifier: Apache-2.0
//

#ifndef PYGPUBENCH_MMAP_MAKE_UNIQUE_H
#define PYGPUBENCH_MMAP_MAKE_UNIQUE_H

#include <memory>
#include <system_error>
#include <sys/mman.h>
#include <unistd.h>


/// Allocates a single object of type T in a dedicated anonymous mmap region so that
/// it occupies its own page(s) and can later be mprotect-ed without affecting any
/// neighbouring heap allocations. The returned unique_ptr destructs the object and
/// munmaps the region automatically.
template<typename T, typename... Args>
std::unique_ptr<T, void(*)(T*)> mmap_make_unique(Args&&... args) {
    const std::size_t page_size = static_cast<std::size_t>(getpagesize());
    const std::size_t alloc_size = (sizeof(T) + page_size - 1) & ~(page_size - 1);

    void* mem = mmap(nullptr, alloc_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        throw std::system_error(errno, std::generic_category(), "mmap failed for BenchmarkManager");
    }

    T* obj = nullptr;
    try {
        obj = new (mem) T(std::forward<Args>(args)...);
    } catch (...) {
        munmap(mem, alloc_size);
        throw;
    }

    auto deleter = [](T* p) {
        const std::size_t page_size = static_cast<std::size_t>(getpagesize());
        const std::size_t alloc_size = (sizeof(T) + page_size - 1) & ~(page_size - 1);
        p->~T();
        munmap(static_cast<void*>(p), alloc_size);
    };

    return {obj, deleter};
}


#endif //PYGPUBENCH_MMAP_MAKE_UNIQUE_H