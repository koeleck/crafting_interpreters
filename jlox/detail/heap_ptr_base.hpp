#pragma once

#include <cstddef>
#include <cassert>
#include <utility>

namespace detail
{

class HeapPtrHead;

class HeapPtrBaseNode
{
public:
    constexpr
    HeapPtrBaseNode() noexcept
      : m_pprev{nullptr}
      , m_next{nullptr}
      , m_ptr{nullptr}
    {}

    constexpr
    ~HeapPtrBaseNode() noexcept
    {
        unlink();
    }

    constexpr
    HeapPtrBaseNode(HeapPtrBaseNode&& other) noexcept
      : m_pprev{other.m_pprev}
      , m_next{other.m_next}
      , m_ptr{other.m_ptr}
    {
        other.m_pprev = nullptr;
        other.m_next = nullptr;
        other.m_ptr = nullptr;
        if (m_pprev) {
            *m_pprev = this;
            if (m_next)
                m_next->m_pprev = &m_next;
        }
    }

    constexpr
    HeapPtrBaseNode& operator=(HeapPtrBaseNode&& other) noexcept
    {
        HeapPtrBaseNode tmp{std::move(other)};
        swap(tmp);
        return *this;
    }

    [[nodiscard]] constexpr
    HeapPtrBaseNode* next() const noexcept
    {
        return m_next;
    }

    [[nodiscard]] constexpr
    void* ptr() const noexcept
    {
        return m_ptr;
    }

    constexpr
    void link(HeapPtrHead& head, void* ptr) noexcept;

    constexpr
    void append(HeapPtrBaseNode& node, void* ptr) noexcept
    {
        assert(ptr != nullptr);
        node.unlink();
        node.m_pprev = &m_next;
        node.m_next = m_next;
        node.m_ptr = ptr;
        if (node.m_next)
            node.m_next->m_pprev = &node.m_next;
        m_next = &node;
    }

    constexpr
    void unlink() noexcept
    {
        if (m_pprev) {
            *m_pprev = m_next;
            if (m_next) {
                m_next->m_pprev = m_pprev;
            }
        }
        m_pprev = nullptr;
        m_next = nullptr;
        m_ptr = nullptr;
    }

    constexpr
    void swap(HeapPtrBaseNode& other) noexcept
    {
        std::swap(m_pprev, other.m_pprev);
        std::swap(m_next, other.m_next);
        std::swap(m_ptr, other.m_ptr);
        if (m_pprev) {
            *m_pprev = this;
            if (m_next)
                m_next->m_pprev = &m_next;
        }
        if (other.m_pprev) {
            *other.m_pprev = &other;
            if (other.m_next)
                other.m_next->m_pprev = &other.m_next;
        }
    }

    HeapPtrBaseNode(const HeapPtrBaseNode&) = delete;
    HeapPtrBaseNode& operator=(const HeapPtrBaseNode&) = delete;
private:
    friend class HeapPtrHead;

    HeapPtrBaseNode** m_pprev;
    HeapPtrBaseNode* m_next;
    void* m_ptr;
};


class HeapPtrHead
{
public:
    constexpr
    HeapPtrHead() noexcept
      : m_first{nullptr}
    {
    }

    constexpr
    ~HeapPtrHead() noexcept
    {
        drop_all();
    }

    constexpr
    HeapPtrHead(HeapPtrHead&& other) noexcept
      : m_first{other.m_first}
    {
        other.m_first = nullptr;
        if (m_first) {
            m_first->m_pprev = &m_first;
        }
    }

    constexpr
    HeapPtrHead& operator=(HeapPtrHead&& other) noexcept
    {
        HeapPtrHead tmp{std::move(other)};
        swap(tmp);
        return *this;
    }

    [[nodiscard]] constexpr
    HeapPtrBaseNode* first() const noexcept
    {
        return m_first;
    }

    constexpr
    void drop_all() noexcept
    {
        while (m_first) {
            m_first->unlink();
        }
    }

    constexpr
    void swap(HeapPtrHead& other) noexcept
    {
        std::swap(m_first, other.m_first);
        if (m_first)
            m_first->m_pprev = &m_first;
        if (other.m_first)
            other.m_first->m_pprev = &other.m_first;
    }

    HeapPtrHead(const HeapPtrHead&) = delete;
    HeapPtrHead& operator=(const HeapPtrHead&) = delete;
private:
    friend class HeapPtrBaseNode;

    HeapPtrBaseNode* m_first;
};

inline constexpr
void HeapPtrBaseNode::link(HeapPtrHead& head, void* ptr) noexcept
{
    assert(ptr != nullptr);
    unlink();
    m_next = head.m_first;
    if (m_next)
        m_next->m_pprev = &m_next;
    head.m_first = this;
    m_pprev = &head.m_first;
    m_ptr = ptr;
}

} // namespace detail
