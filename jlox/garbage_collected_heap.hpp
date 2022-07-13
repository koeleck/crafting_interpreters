#include <cassert>
#include <vector>
#include <new>

#include "detail/heap_ptr_base.hpp"

class GarbageCollectedHeap;

template <typename T>
class HeapPtr : private detail::HeapPtrBaseNode
{
public:
    using detail::HeapPtrBaseNode::swap;

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
    T& operator*() const noexcept
    {
        assert(this->ptr());
        return *static_cast<T*>(this->ptr());
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

private:
    friend class GarbageCollectedHeap;
};

class GarbageCollectedHeap
{
public:
    inline static constexpr std::size_t ALLOC_GRANULARITY = 32;

    explicit
    GarbageCollectedHeap(std::size_t capacity);

    ~GarbageCollectedHeap();

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

    /**
     * \returns number of free bytes (complexity: O(n))
     */
    std::size_t num_free_bytes() const noexcept;

    constexpr
    std::size_t capacity() const noexcept
    {
        return m_capacity;
    }

    GarbageCollectedHeap(const GarbageCollectedHeap&) = delete;
    GarbageCollectedHeap(GarbageCollectedHeap&&) = delete;
    GarbageCollectedHeap& operator=(const GarbageCollectedHeap&) = delete;
    GarbageCollectedHeap& operator=(GarbageCollectedHeap&&) = delete;
private:
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

    std::vector<AllocatedBlock> m_allocated;
    std::vector<FreeBlock> m_free;

    char* m_memory;
    std::size_t m_capacity;
};

