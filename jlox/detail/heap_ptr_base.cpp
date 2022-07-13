#include "heap_ptr_base.hpp"

#include <doctest/doctest.h>


TEST_CASE("HeapPtrBase")
{
    using namespace detail;

    HeapPtrBaseNode ptr;
    HeapPtrHead head;

    REQUIRE(ptr.next() == nullptr);
    REQUIRE(ptr.ptr() == nullptr);
    REQUIRE(head.first() == nullptr);

    SUBCASE("Link pointers") {
        int tmp{12};
        ptr.link(head, &tmp);

        REQUIRE(head.first() == &ptr);
        REQUIRE(ptr.next() == nullptr);
        REQUIRE(ptr.ptr() == &tmp);

        HeapPtrBaseNode ptr2;
        int tmp2{23};
        ptr2.link(head, &tmp2);
        REQUIRE(head.first() == &ptr2);
        REQUIRE(ptr2.next() == &ptr);
        REQUIRE(ptr2.ptr() == &tmp2);

        ptr2.unlink();
        REQUIRE(ptr2.next() == nullptr);
        REQUIRE(ptr2.ptr() == nullptr);
        REQUIRE(head.first() == &ptr);
        REQUIRE(ptr.next() == nullptr);
        REQUIRE(ptr.ptr() == &tmp);
    }

    SUBCASE("Append pointers") {
        HeapPtrBaseNode ptr1;
        HeapPtrBaseNode ptr2;
        int tmp{1};
        ptr1.append(ptr2, &tmp);

        REQUIRE(ptr1.next() == &ptr2);
        REQUIRE(ptr2.ptr() == &tmp);
    }

    SUBCASE("Drop all from head") {
        int tmp{45};
        ptr.link(head, &tmp);

        HeapPtrBaseNode ptr2;
        ptr2.link(head, &tmp);

        HeapPtrBaseNode ptr3;
        ptr3.link(head, &tmp);

        REQUIRE(head.first() == &ptr3);

        head.drop_all();

        REQUIRE(head.first() == nullptr);
        REQUIRE(ptr.ptr() == nullptr);
        REQUIRE(ptr.next() == nullptr);
        REQUIRE(ptr2.ptr() == nullptr);
        REQUIRE(ptr2.next() == nullptr);
        REQUIRE(ptr3.ptr() == nullptr);
        REQUIRE(ptr3.next() == nullptr);
    }
}
