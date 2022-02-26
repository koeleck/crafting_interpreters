#pragma once

#include <cstdint>
#include <vector>
#include <type_traits>
#include <new>

class BumpAlloc
{
public:
    inline static constexpr int32_t BLOCK_SIZE = 16 * 1024;
    struct State {
        uint32_t offset;
        uint32_t block;
    };

    BumpAlloc();
    ~BumpAlloc();
    BumpAlloc(BumpAlloc&&) noexcept;
    BumpAlloc& operator=(BumpAlloc&&) noexcept;

    void reset() noexcept;
    void reset(State state) noexcept;

    State get_state() const noexcept {
        return m_state;
    }

    template <typename T, typename... Args>
    T* allocate(Args&&... args)
    {
        if constexpr (std::is_trivially_destructible_v<T>) {
            if constexpr (noexcept(T{std::forward<Args>(args)...})) {
                return new(raw_allocate(sizeof(T), alignof(T))) T{std::forward<Args>(args)...};
            } else {
                AllocationHelper helper{*this, sizeof(T), alignof(T)};
                new (helper.ptr()) T{std::forward<Args>(args)...};
                return helper.release<T>();
            }
        } else {
            using VWD = ValueWithDeleter<T>;
            if constexpr (noexcept(VWD{&m_deleter, std::forward<Args>(args)...})) {
                VWD* result = new(raw_allocate(sizeof(VWD), alignof(VWD))) VWD{&m_deleter, std::forward<Args>(args)...};
                return std::addressof(result->value);
            } else {
                AllocationHelper helper{*this, sizeof(VWD), alignof(VWD)};
                VWD* result = new (helper.ptr()) VWD{&m_deleter, std::forward<Args>(args)...};
                return std::addressof(helper.release<VWD>()->value);
            }
        }
    }

    void swap(BumpAlloc&) noexcept;

private:
    struct Block;

    struct Deleter
    {
        void (*dtor)(Deleter*) noexcept;
        Deleter* next;
    };

    template <typename T>
    struct ValueWithDeleter : Deleter
    {
        T value;

        template <typename... Args>
        constexpr
        ValueWithDeleter(Deleter** head, Args&&... args) noexcept(noexcept(T{std::forward<Args>(args)...}))
          : Deleter{&ValueWithDeleter::destroy, *head}
          , value{std::forward<Args>(args)...}
        {
            *head = this;
        }

        static constexpr
        void destroy(Deleter* const deleter) noexcept
        {
            ValueWithDeleter* const this_ptr = static_cast<ValueWithDeleter*>(deleter);
            this_ptr->~ValueWithDeleter();
        }
    };

    class AllocationHelper {
    public:
        AllocationHelper(BumpAlloc& alloc, uint32_t size, uint32_t alignment)
          : m_alloc{alloc}
          , m_backup{alloc.get_state()}
          , m_ptr{alloc.raw_allocate(size, alignment)}
        {}

        ~AllocationHelper()
        {
            if (m_ptr) {
                m_alloc.reset(m_backup);
            }
        }

        void* ptr() const noexcept
        {
            return m_ptr;
        }

        template <typename T>
        T* release() noexcept
        {
            void* const tmp = m_ptr;
            m_ptr = nullptr;
            return static_cast<T*>(tmp);
        }

    private:
        BumpAlloc& m_alloc;
        State m_backup;
        void* m_ptr;
    };
    friend class AllocationHelper;

    void* raw_allocate(uint32_t size, uint32_t alignment);

    State m_state;
    std::vector<Block> m_blocks;
    Deleter* m_deleter;
};
