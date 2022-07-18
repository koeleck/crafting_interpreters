#pragma once

#include <cassert>
#include <vector>
#include <new>

#include "detail/heap_ptr_base.hpp"

class GarbageCollectedHeap;

template <typename T>
class HeapPtr : private detail::HeapPtrBaseNode
{
public:
    using element_type = T;
    using difference_type = std::ptrdiff_t;

    template <typename U=T>
    [[nodiscard]] static constexpr
    HeapPtr<T> pointer_to(U& r) noexcept requires (!std::is_void_v<T> && std::is_same_v<T, U>);

    [[nodiscard]] static constexpr
    T* to_address(const HeapPtr<T>& ptr) noexcept
    {
        return ptr.get();
    }


    constexpr
    HeapPtr() noexcept = default;

    constexpr
    HeapPtr(std::nullptr_t) noexcept
      : HeapPtr{}
    {}

    constexpr
    HeapPtr(const HeapPtr& other) noexcept
      : HeapPtr{}
    {
        if (other.ptr()) {
            const_cast<HeapPtr&>(other).append(*this, other.ptr());
            assert(this->ptr() == other.ptr());
        }
    }

    constexpr
    HeapPtr(HeapPtr&& other) noexcept
      : HeapPtrBaseNode{static_cast<HeapPtrBaseNode&&>(other)}
    {}

    template <typename U, typename = std::enable_if_t<std::is_convertible_v<std::add_pointer_t<U>, std::add_pointer_t<T>>>>
    constexpr
    HeapPtr(const HeapPtr<U>& other) noexcept
      : HeapPtr{}
    {
        if (other.ptr()) {
            const_cast<HeapPtr<U>&>(other).append(*this, other.ptr());
            assert(static_cast<U*>(this->ptr()) == other.ptr());
        }
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible_v<std::add_pointer_t<U>, std::add_pointer_t<T>>>>
    constexpr
    HeapPtr(HeapPtr<U>&& other) noexcept
      : HeapPtrBaseNode{static_cast<HeapPtrBaseNode&&>(other)}
    {}

    constexpr
    HeapPtr& operator=(const HeapPtr& other) noexcept
    {
        HeapPtr tmp{other};
        swap(tmp);
        return *this;
    }

    constexpr
    HeapPtr& operator=(HeapPtr&& other) noexcept
    {
        HeapPtr tmp{std::move(other)};
        swap(tmp);
        return *this;
    }

    constexpr
    ~HeapPtr() noexcept = default;

    [[nodiscard]] constexpr
    bool operator!() const noexcept
    {
        return this->ptr() == nullptr;
    }

    [[nodiscard]] constexpr
    operator bool() const noexcept
    {
        return this->ptr() != nullptr;
    }

    [[nodiscard]] constexpr
    T* operator->() const noexcept
    {
        assert(this->ptr());
        return static_cast<T*>(this->ptr());
    }

    [[nodiscard]] constexpr
    auto operator*() const noexcept -> std::add_lvalue_reference_t<T> requires (!std::is_void_v<T>)
    {
        assert(this->ptr());
        return *static_cast<T*>(this->ptr());
    }

    [[nodiscard]] constexpr
    T* get() const noexcept
    {
        return static_cast<T*>(this->ptr());
    }

    [[nodiscard]] constexpr
    bool operator==(const HeapPtr& other) const noexcept
    {
        return this->ptr() == other.ptr();
    }

    [[nodiscard]] constexpr
    bool operator!=(const HeapPtr& other) const noexcept
    {
        return this->ptr() != other.ptr();
    }

    constexpr
    void reset() noexcept
    {
        unlink();
    }

    using detail::HeapPtrBaseNode::swap;

private:
    template <typename U>
    friend class HeapPtr;


    template <typename U>
    [[nodiscard]] friend
    HeapPtr<U> static_heap_pointer_cast(const HeapPtr& ptr) noexcept
    {
        HeapPtr<U> result;
        if (ptr.ptr()) {
            const_cast<HeapPtr&>(ptr).append(result, static_cast<U*>(ptr.ptr()));
        }
        return result;
    }

    friend class GarbageCollectedHeap;
};

class GarbageCollectedHeap
{
public:
    inline static constexpr std::size_t ALLOC_GRANULARITY = 32;

    void run_gc() noexcept;

    template <typename T, typename... Args>
    HeapPtr<T> allocate(Args&&... args)
    {
        static_assert(alignof(T) <= ALLOC_GRANULARITY);
        static constexpr std::size_t allocation_size = (sizeof(T) + (ALLOC_GRANULARITY - 1)) & -(ALLOC_GRANULARITY);

        auto it = allocate_raw(allocation_size);

        T* ptr{nullptr};
        try {
            ptr = new (m_memory + it->offset) T(std::forward<Args>(args)...);
        } catch (...) {
            undo_raw_allocation(it);
            throw;
        }


        HeapPtr<T> heap_ptr;
        heap_ptr.link(it->referenced_by, ptr);
        if constexpr (std::is_trivially_destructible_v<T> == false) {
            it->dtor = [](void* ptr) noexcept {
                static_cast<T*>(ptr)->~T();
            };
        }
        return heap_ptr;
    }


    HeapPtr<void> allocate_bytes(std::size_t n);

    /**
     * \returns number of free bytes (complexity: O(n))
     */
    std::size_t num_free_bytes() const noexcept;

    template <typename T>
    HeapPtr<T> reference_to_allocation(T* ptr) noexcept
    {
        return static_heap_pointer_cast<T>(this->reference_to_allocation_impl(static_cast<const void*>(ptr)));
    }

    constexpr
    std::size_t capacity() const noexcept
    {
        return m_capacity;
    }

    [[nodiscard]] static
    GarbageCollectedHeap& get_heap() noexcept;

    GarbageCollectedHeap(const GarbageCollectedHeap&) = delete;
    GarbageCollectedHeap(GarbageCollectedHeap&&) = delete;
    GarbageCollectedHeap& operator=(const GarbageCollectedHeap&) = delete;
    GarbageCollectedHeap& operator=(GarbageCollectedHeap&&) = delete;
private:
    explicit
    GarbageCollectedHeap(std::size_t capacity);

    ~GarbageCollectedHeap();

    struct AllocatedBlock
    {
        std::size_t offset{0};
        std::size_t size{0};
        detail::HeapPtrHead referenced_by;
        std::vector<AllocatedBlock*> references;
        void (*dtor)(void*) noexcept{nullptr};
        bool alive{false};
        bool visited{false};
    };

    struct FreeBlock
    {
        std::size_t offset{0};
        std::size_t size{0};
    };

    std::vector<AllocatedBlock>::iterator allocate_raw(std::size_t size);
    void undo_raw_allocation(std::vector<AllocatedBlock>::iterator it) noexcept;
    void propagate_aliveness(AllocatedBlock& to) noexcept;
    HeapPtr<void> reference_to_allocation_impl(const void* ptr) noexcept;

    std::vector<AllocatedBlock> m_allocated;
    std::vector<FreeBlock> m_free;

    char* m_memory;
    std::size_t m_capacity;
};

struct Heap
{
    static
    void run_gc() noexcept
    {
        GarbageCollectedHeap::get_heap().run_gc();
    }

    template <typename T, typename... Args>
    [[nodiscard]] static
    HeapPtr<T> allocate(Args&&... args)
    {
        return GarbageCollectedHeap::get_heap().allocate<T>(std::forward<Args>(args)...);
    }

    [[nodiscard]] static
    HeapPtr<void> allocate_bytes(std::size_t n)
    {
        return GarbageCollectedHeap::get_heap().allocate_bytes(n);
    }

    [[nodiscard]] static
    std::size_t num_free_bytes() noexcept
    {
        return GarbageCollectedHeap::get_heap().num_free_bytes();
    }

    template <typename T>
    [[nodiscard]] static
    HeapPtr<T> reference_to_allocation(T* ptr) noexcept
    {
        return GarbageCollectedHeap::get_heap().reference_to_allocation<T>(ptr);
    }

    [[nodiscard]] static
    std::size_t capacity() noexcept
    {
        return GarbageCollectedHeap::get_heap().capacity();
    }
};

template <typename T>
template <typename U>
[[nodiscard]] constexpr
HeapPtr<T> HeapPtr<T>::pointer_to(U& r) noexcept requires (!std::is_void_v<T> && std::is_same_v<T, U>)
{
    return GarbageCollectedHeap::get_heap().reference_to_allocation<U>(std::addressof(r));
}


template <typename T>
class GarbageCollectedAllocator
{
public:
    using value_type = T;
    using pointer = HeapPtr<T>;
    using const_pointer = HeapPtr<const T>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using is_always_equal = std::true_type;

    GarbageCollectedAllocator() = default;


    template <typename U>
    constexpr
    GarbageCollectedAllocator(const GarbageCollectedAllocator<U>&) noexcept
    {}

    [[nodiscard]]
    pointer allocate(size_type n)
    {
        return static_heap_pointer_cast<T>(Heap::allocate_bytes(n * sizeof(T)));
    }

    constexpr
    void deallocate(pointer /*p*/, size_type /*n*/) const noexcept
    {
    }
};
