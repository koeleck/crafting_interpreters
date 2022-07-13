#include "garbage_collected_heap.hpp"

#include <sys/mman.h>
#include <cstdlib>
#include <algorithm>
#include <cstdio>

static
constexpr std::size_t PAGE_SIZE = 4 * 1024;

GarbageCollectedHeap::GarbageCollectedHeap(std::size_t capacity)
  : m_memory{nullptr}
  , m_capacity{0}
{
    capacity = ((capacity + (PAGE_SIZE - 1)) / PAGE_SIZE) * PAGE_SIZE;

    void* const memory = mmap(nullptr, capacity, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        std::fputs("Failed to mmap memory.", stderr);
        std::abort();
    }
    m_memory = static_cast<char*>(memory);
    m_capacity = capacity;

    FreeBlock b;
    b.size = m_capacity;
    m_free.push_back(std::move(b));
}

GarbageCollectedHeap::~GarbageCollectedHeap()
{
    run_gc();
    if (m_allocated.empty() == false) {
        std::fputs("Stuff is still allocated.", stderr);
        std::abort();
    }
    if (0 != munmap(m_memory, m_capacity)) {
        std::fputs("munmap failed.", stderr);
        std::abort();
    }
}

void GarbageCollectedHeap::run_gc() noexcept
{
    // - mark all nodes as dead
    // - remove all edges between nodes
    for (auto& b : m_allocated) {
        b.alive = false;
        b.visited = false;
        b.references.clear();
    }

    // - mark all blocks with external references as alive.
    // - build graph in case of internal references.
    for (auto& this_block : m_allocated) {
        const detail::HeapPtrBaseNode* ref = this_block.referenced_by.first();
        while (ref) {
            const char* const ptr = reinterpret_cast<const char*>(ref);
            if (ptr < m_memory || (m_memory + m_capacity) <= ptr) {
                this_block.alive = true;
            } else {
                const std::size_t offset = ptr - m_memory;
                const auto src = std::lower_bound(
                        m_allocated.begin(),
                        m_allocated.end(),
                        offset,
                        [] (const AllocatedBlock& block, std::size_t offset)
                        {
                            return block.offset < offset;
                        });
                assert(src != m_allocated.end());
                src->references.push_back(&this_block);
            }
            ref = ref->next();
        }
    }

    // propagate aliveness
    for (auto& b : m_allocated) {
        if (b.visited || !b.alive)
            continue;
        for (AllocatedBlock* next : b.references)
            propagate_aliveness(*next);
    }

    // erase all orphaned blocks
    bool freed_something = false;
    {
        std::size_t n = m_allocated.size();
        while (n-- > 0) {
            AllocatedBlock& block = m_allocated[n];
            if (!block.alive) {
                if (block.dtor)
                    block.dtor(m_memory + block.offset);
                freed_something = true;
                m_free.push_back(FreeBlock{block.offset, block.size});
                m_allocated.erase(m_allocated.begin() + n);
            }
        }
    }

    // maintain freelist
    if (freed_something) {
        assert(m_free.empty() == false);
        std::sort(m_free.begin(), m_free.end(),
                [] (const FreeBlock& lhs, const FreeBlock& rhs) {
                    return lhs.offset < rhs.offset;
                });
        std::size_t n = m_free.size();
        while (n-- > 1) {
            FreeBlock& cur = m_free[n];
            FreeBlock& prev = m_free[n - 1];
            if ((prev.offset + prev.size) == cur.offset) {
                prev.size += cur.size;
                m_free.erase(m_free.begin() + n);
            }
        }
    }
}

void GarbageCollectedHeap::propagate_aliveness(GarbageCollectedHeap::AllocatedBlock& to) noexcept
{
    if (to.visited)
        return;
    to.visited = true;
    to.alive = true;
    for (AllocatedBlock* next : to.references) {
        propagate_aliveness(*next);
    }
}

std::vector<GarbageCollectedHeap::AllocatedBlock>::iterator
GarbageCollectedHeap::allocate_raw(const std::size_t size)
{
    assert((size % ALLOC_GRANULARITY) == 0);

    // TODO: better approach than first-fit.
    const auto find_next_best_block = [&] () {
        auto it = m_free.begin();
        while (it != m_free.end()) {
            if (it->size >= size)
                break;
        }
        return it;
    };

    auto free_block = find_next_best_block();
    if (free_block == m_free.end()) {
        run_gc();
        free_block = find_next_best_block();
    }
    if (free_block == m_free.end())
        throw std::bad_alloc{};

    const auto ins_point = std::lower_bound(
            m_allocated.begin(),
            m_allocated.end(),
            free_block->offset,
            [] (const AllocatedBlock& b, std::size_t offs) {
                return b.offset < offs;
            });

    const auto new_block_on_the_block = m_allocated.emplace(ins_point);
    new_block_on_the_block->offset = free_block->offset;
    new_block_on_the_block->size = size;
    if (free_block->size > size) {
        free_block->size -= size;
        free_block->offset += size;
    } else {
        m_free.erase(free_block);
    }

    return new_block_on_the_block;
}

void GarbageCollectedHeap::undo_raw_allocation(std::vector<GarbageCollectedHeap::AllocatedBlock>::iterator undo_alloc) noexcept
{
    // A previous raw allocation could either have split the original free block or completely removed it
    const std::size_t free_block_offset = undo_alloc->offset + undo_alloc->size;
    const auto free_block = std::lower_bound(
            m_free.begin(), m_free.end(),
            free_block_offset,
            [] (const FreeBlock& block, std::size_t free_block_offset) {
                return block.offset < free_block_offset;
            });

    if (free_block == m_free.end() || free_block->offset != free_block_offset) {
        // free block was removed. add again.
        m_free.emplace(free_block, FreeBlock{undo_alloc->offset, undo_alloc->size});
    } else {
        // restore original size and offset
        free_block->offset = undo_alloc->offset;
        free_block->size += undo_alloc->size;
    }

    m_allocated.erase(undo_alloc);
}


std::size_t GarbageCollectedHeap::num_free_bytes() const noexcept
{
    std::size_t result{0};
    for (const FreeBlock& block : m_free) {
        result += block.size;
    }
    return result;
}

///////////////////////////////////////////////////////////////
#include <doctest/doctest.h>
#include <stdexcept>

TEST_CASE("GarbageCollectedHeap")
{
    GarbageCollectedHeap heap{2 * 1024 * 1024};
    REQUIRE(heap.num_free_bytes() == heap.capacity());

    SUBCASE("External references only") {
        {
            HeapPtr<int> ptr1 = heap.allocate<int>(12);
            REQUIRE(*ptr1 == 12);
            const std::size_t num_free_1 = heap.num_free_bytes();
            REQUIRE(num_free_1 < heap.capacity());

            heap.run_gc();
            REQUIRE(heap.num_free_bytes() == num_free_1);
            {
                HeapPtr<int> ptr2 = heap.allocate<int>(13);
                REQUIRE(*ptr2 == 13);
                const std::size_t num_free_2 = heap.num_free_bytes();
                REQUIRE(num_free_2 < num_free_1);

                heap.run_gc();
                REQUIRE(heap.num_free_bytes() == num_free_2);
            }

            heap.run_gc();
            REQUIRE(heap.num_free_bytes() == num_free_1);
        }
        heap.run_gc();
        REQUIRE(heap.num_free_bytes() == heap.capacity());
    }

    SUBCASE("Allocation with multiple references") {
        {
            HeapPtr<int> ptr1 = heap.allocate<int>(12);
            REQUIRE(*ptr1 == 12);
            const std::size_t num_free = heap.num_free_bytes();
            REQUIRE(num_free < heap.capacity());
            {
                HeapPtr<int> ptr2 = ptr1;
                REQUIRE(ptr1 == ptr2);
                REQUIRE(*ptr2 == 12);

                heap.run_gc();
                REQUIRE(heap.num_free_bytes() == num_free);
            }
            heap.run_gc();
            REQUIRE(heap.num_free_bytes() == num_free);
        }
        heap.run_gc();
        REQUIRE(heap.num_free_bytes() == heap.capacity());
    }

    SUBCASE("Internal and external references") {
        struct Chain {
            HeapPtr<Chain> next;
            int value;
        };

        auto alloc = [&] (HeapPtr<Chain> next) {
            const int v = (next) ? (next->value + 1) : 0;
            return heap.allocate<Chain>(std::move(next), v);
        };
        HeapPtr<Chain> root;
        for (int i = 0; i < 100; ++i) {
            root = alloc(std::move(root));
        }

        const std::size_t num_free = heap.num_free_bytes();
        REQUIRE(num_free < heap.capacity());
        root.reset();
        heap.run_gc();
        REQUIRE(heap.num_free_bytes() == heap.capacity());
    }

    SUBCASE("Throwing constructor should not leak") {
        struct ThrowsInCtor {
            ThrowsInCtor()
            {
                throw std::runtime_error{""};
            }
        };

        try {
            heap.allocate<ThrowsInCtor>();
        } catch (const std::runtime_error&) {
            REQUIRE(heap.num_free_bytes() == heap.capacity());
        } catch (...) {
            REQUIRE(false);
        }
    }

    SUBCASE("Run out of memory") {
        std::vector<HeapPtr<std::size_t>> ptrs;

        try {
            for (std::size_t i = 0;; ++i) {
                HeapPtr<std::size_t> ptr = heap.allocate<std::size_t>(i);
                ptrs.push_back(std::move(ptr));
            }
        } catch (const std::bad_alloc&) {
        } catch (...) {
            REQUIRE(false);
        }

        REQUIRE(heap.num_free_bytes() == 0);
        ptrs.clear();
        heap.run_gc();
        REQUIRE(heap.num_free_bytes() == heap.capacity());
    }
}
