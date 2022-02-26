#include "bump_alloc.hpp"

#include <cstdlib>
#include <cstddef>
#include <memory>
#include <cassert>

namespace
{

struct FreeDeleter
{
    constexpr
    void operator()(void* ptr) const noexcept
    {
        std::free(ptr);
    }
};

bool in_range(const void* ptr, const void* begin, uint32_t size) noexcept
{
    const uintptr_t p = reinterpret_cast<uintptr_t>(ptr);
    const uintptr_t b = reinterpret_cast<uintptr_t>(begin);
    return (p >= b) && (p < (b + size));
}

} // anonymous namespace

struct BumpAlloc::Block
  : public std::unique_ptr<char[], FreeDeleter>
{
    Block(void* ptr) noexcept
      : std::unique_ptr<char[], FreeDeleter>{static_cast<char*>(ptr)}
    {}
};

BumpAlloc::BumpAlloc()
  : m_state{0, -1u}
  , m_blocks{}
  , m_deleter{nullptr}
{}

BumpAlloc::~BumpAlloc()
{
    reset();
}

BumpAlloc::BumpAlloc(BumpAlloc&& other) noexcept
  : m_state{other.m_state}
  , m_blocks{std::move(other.m_blocks)}
  , m_deleter{other.m_deleter}
{
    other.m_state = State{0, -1u};
    other.m_deleter = nullptr;
}

BumpAlloc& BumpAlloc::operator=(BumpAlloc&& other) noexcept
{
    BumpAlloc tmp{std::move(other)};
    swap(tmp);
    return *this;
}

void BumpAlloc::reset() noexcept
{
    Deleter* deleter = m_deleter;
    while (deleter) {
        Deleter* const next = deleter->next;
        deleter->dtor(deleter);
        deleter = next;
    }

    m_deleter = nullptr;
    if (m_blocks.empty()) {
        m_state.block = -1u;
        m_state.offset = 0;
    } else {
        m_state.block = 0;
        m_state.offset = BLOCK_SIZE;
    }
}

void BumpAlloc::reset(const State state) noexcept
{
    assert((state.block < m_state.block && state.offset <= BLOCK_SIZE) ||
           (state.block == m_state.block && state.offset <= m_state.offset));

    Deleter* deleter = m_deleter;
    uint32_t block_idx = m_state.block;
    while (block_idx > state.block) {
        const char* const block = m_blocks[block_idx].get();
        while (deleter && in_range(deleter, block, BLOCK_SIZE)) {
            Deleter* const next = deleter->next;
            deleter->dtor(deleter);
            deleter = next;
        }
        --block_idx;
    }

    assert(block_idx == state.block);
    const char* const block = m_blocks[block_idx].get();
    while (deleter && in_range(deleter, block, state.offset)) {
        Deleter* const next = deleter->next;
        deleter->dtor(deleter);
        deleter = next;
    }
    m_deleter = deleter;
    m_state = state;
}

void BumpAlloc::swap(BumpAlloc& other) noexcept
{
    std::swap(m_state, other.m_state);
    m_blocks.swap(other.m_blocks);
    std::swap(m_deleter, other.m_deleter);
}


void* BumpAlloc::raw_allocate(const uint32_t initial_size,
                              const uint32_t alignment)
{
    assert((alignment & (alignment - 1)) == 0);
    assert(alignment <= alignof(std::max_align_t));

    const uint32_t size = std::max(initial_size, 1u);
    if (size > BLOCK_SIZE) {
        std::abort();
    }

    uint32_t new_offset = (m_state.offset - size) & -alignment;
    if (new_offset < BLOCK_SIZE) {
        m_state.offset = new_offset;
        return m_blocks[m_state.block].get() + new_offset;
    }

    if (m_state.block + 1 == m_blocks.size()) {
        m_blocks.emplace_back(std::aligned_alloc(alignof(std::max_align_t), BLOCK_SIZE));
    }
    m_state.block++;
    m_state.offset = (BLOCK_SIZE - size) & -alignment;
    assert(m_state.offset < BLOCK_SIZE);
    return m_blocks[m_state.block].get() + m_state.offset;
}



//
#include <doctest/doctest.h>

namespace
{

class Signal
{
public:
    Signal() = default;

    constexpr explicit
    Signal(int32_t* signal) noexcept
      : m_signal{signal}
    {}

    ~Signal()
    {
        if (m_signal) {
            ++*m_signal;
        }
    }

    Signal(const Signal&) = delete;
    Signal(Signal&&) = delete;
    Signal& operator=(const Signal&) = delete;
    Signal& operator=(Signal&&) = delete;
private:
    int32_t* m_signal{nullptr};
};

TEST_CASE("Bump allocator tests")
{
    static constexpr int32_t SIGNAL_SIZE = 24; // m_signal + deleter (2x pointer)
    static constexpr int32_t SIGNALS_PER_BLOCK = BumpAlloc::BLOCK_SIZE / SIGNAL_SIZE;

    BumpAlloc alloc;
    int32_t counter{0};

    for (int32_t i = 0; i < SIGNALS_PER_BLOCK + 5; ++i) {
        alloc.allocate<Signal>(&counter);
    }

    {
        BumpAlloc::State state = alloc.get_state();

        CHECK(state.block == 1);
        CHECK(state.offset == BumpAlloc::BLOCK_SIZE - 5 * SIGNAL_SIZE);
    }


    {
        CHECK(counter == 0);

        BumpAlloc::State state{10 * SIGNAL_SIZE, 0};
        alloc.reset(state);

        CHECK(counter == 15);
    }

    alloc.reset();
    CHECK(counter == SIGNALS_PER_BLOCK + 5);
}

} // anonymous namespace
