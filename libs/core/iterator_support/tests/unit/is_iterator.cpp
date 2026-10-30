//  Copyright (c) 2019 Austin McCartney
//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <forward_list>
#include <iterator>
#include <list>
#include <type_traits>
#include <vector>

namespace test {

    template <typename BaseIterator, typename IteratorTag>
    struct test_iterator
      : hpx::util::iterator_adaptor<test_iterator<BaseIterator, IteratorTag>,
            BaseIterator, void, IteratorTag>
    {
    private:
        using base_type = hpx::util::iterator_adaptor<
            test_iterator<BaseIterator, IteratorTag>, BaseIterator, void,
            IteratorTag>;

    public:
        // disable use of brackets_proxy in iterator_facade
        using use_brackets_proxy = std::false_type;

        test_iterator() = default;

        explicit constexpr test_iterator(BaseIterator base)
          : base_type(base)
        {
        }
    };
}    // namespace test

template <typename T, typename = void>
struct has_nested_type : std::integral_constant<bool, false>
{
};

template <typename T>
struct has_nested_type<T, std::void_t<typename T::type>>
  : std::integral_constant<bool, true>
{
};

struct bidirectional_traversal_iterator
{
    using difference_type = int;
    using value_type = int;
    using iterator_category = std::bidirectional_iterator_tag;
    using pointer = int const*;
    using reference = int;

    int state;

    reference operator*() const
    {
        return state;
    }

    pointer operator->() const = delete;

    bidirectional_traversal_iterator& operator++()
    {
        ++state;
        return *this;
    }

    bidirectional_traversal_iterator operator++(int)
    {
        bidirectional_traversal_iterator copy = *this;
        ++(*this);
        return copy;
    }

    bidirectional_traversal_iterator& operator--()
    {
        --state;
        return *this;
    }

    bidirectional_traversal_iterator operator--(int)
    {
        bidirectional_traversal_iterator copy = *this;
        --(*this);
        return copy;
    }

    bool operator==(bidirectional_traversal_iterator const& that) const
    {
        return state == that.state;
    }

    bool operator!=(bidirectional_traversal_iterator const& that) const
    {
        return state != that.state;
    }
};

struct random_access_traversal_iterator
{
    using difference_type = int;
    using value_type = int;
    using iterator_category = std::random_access_iterator_tag;
    using pointer = int const*;
    using reference = int;

    int state;

    reference operator*() const
    {
        return state;
    }

    pointer operator->() const = delete;

    random_access_traversal_iterator& operator++()
    {
        ++state;
        return *this;
    }

    random_access_traversal_iterator operator++(int)
    {
        random_access_traversal_iterator copy = *this;
        ++(*this);
        return copy;
    }

    random_access_traversal_iterator& operator--()
    {
        --state;
        return *this;
    }

    random_access_traversal_iterator operator--(int)
    {
        random_access_traversal_iterator copy = *this;
        --(*this);
        return copy;
    }

    reference operator[](difference_type n) const
    {
        return state + n;
    }

    random_access_traversal_iterator& operator+=(difference_type n)
    {
        state += n;
        return *this;
    }

    random_access_traversal_iterator operator+(difference_type n) const
    {
        random_access_traversal_iterator copy = *this;
        return copy += n;
    }

    random_access_traversal_iterator& operator-=(difference_type n)
    {
        state -= n;
        return *this;
    }

    random_access_traversal_iterator operator-(difference_type n) const
    {
        random_access_traversal_iterator copy = *this;
        return copy -= n;
    }

    difference_type operator-(
        random_access_traversal_iterator const& that) const
    {
        return state - that.state;
    }

    bool operator==(random_access_traversal_iterator const& that) const
    {
        return state == that.state;
    }

    bool operator!=(random_access_traversal_iterator const& that) const
    {
        return state != that.state;
    }

    bool operator<(random_access_traversal_iterator const& that) const
    {
        return state < that.state;
    }

    bool operator<=(random_access_traversal_iterator const& that) const
    {
        return state <= that.state;
    }

    bool operator>(random_access_traversal_iterator const& that) const
    {
        return state > that.state;
    }

    bool operator>=(random_access_traversal_iterator const& that) const
    {
        return state >= that.state;
    }

    friend random_access_traversal_iterator operator+(
        difference_type n, random_access_traversal_iterator it)
    {
        return it += n;
    }
};

template <typename T, typename U>
concept addition_result =
    requires { typename hpx::traits::detail::addition_result_t<T, U>; };

template <typename T>
concept dereference_result =
    requires { typename hpx::traits::detail::dereference_result_t<T>; };

template <typename T, typename U>
concept inplace_addition_result =
    requires { typename hpx::traits::detail::inplace_addition_result_t<T, U>; };

template <typename T, typename U>
concept subtraction_result =
    requires { typename hpx::traits::detail::subtraction_result_t<T, U>; };

template <typename T, typename U>
concept inplace_subtraction_result = requires {
    typename hpx::traits::detail::inplace_subtraction_result_t<T, U>;
};

template <typename T>
concept predecrement_result =
    requires { typename hpx::traits::detail::predecrement_result_t<T>; };

template <typename T>
concept preincrement_result =
    requires { typename hpx::traits::detail::preincrement_result_t<T>; };

template <typename T>
concept postdecrement_result =
    requires { typename hpx::traits::detail::postdecrement_result_t<T>; };

template <typename T>
concept postincrement_result =
    requires { typename hpx::traits::detail::postincrement_result_t<T>; };

template <typename T, typename U>
concept subscript_result =
    requires { typename hpx::traits::detail::subscript_result_t<T, U>; };

void addition_result_test()
{
    using namespace hpx::traits::detail;

    struct A
    {
    };
    struct B
    {
    };

    struct C
    {
        B operator+(A const&) const
        {
            return B{};
        }
    };

    HPX_TEST_MSG((std::is_same_v<B, addition_result_t<C, A>>), "deduced type");

    HPX_TEST_MSG((!addition_result<B, A>), "invalid operation");
}

void dereference_result_test()
{
    using namespace hpx::traits::detail;

    struct A
    {
    };

    struct B
    {
        A operator*() const
        {
            return A{};
        }
    };

    HPX_TEST_MSG((std::is_same_v<A, dereference_result_t<B>>), "deduced type");

    HPX_TEST_MSG((!dereference_result<A>), "invalid operation");
}

void equality_result_test()
{
    using namespace hpx::traits::detail;

    struct A
    {
    };
    struct B
    {
    };

    struct C
    {
        B operator==(A const&) const
        {
            return B{};
        }
    };

    HPX_TEST_MSG((std::is_same_v<B, equality_result_t<C, A>>), "deduced type");

    HPX_TEST_MSG(
        (!has_nested_type<equality_result<B, A>>::value), "invalid operation");
}

void inequality_result_test()
{
    using namespace hpx::traits::detail;

    struct A
    {
    };
    struct B
    {
    };

    struct C
    {
        B operator!=(A const&) const
        {
            return B{};
        }
    };

    HPX_TEST_MSG(
        (std::is_same_v<B, inequality_result_t<C, A>>), "deduced type");

    HPX_TEST_MSG((!has_nested_type<inequality_result<B, A>>::value),
        "invalid operation");
}

void inplace_addition_result_test()
{
    using namespace hpx::traits::detail;

    struct A
    {
    };
    struct B
    {
    };

    struct C
    {
        B operator+=(A const&) const
        {
            return B{};
        }
    };

    HPX_TEST_MSG(
        (std::is_same_v<B, inplace_addition_result_t<C, A>>), "deduced type");

    HPX_TEST_MSG((!inplace_addition_result<B, A>), "invalid operation");
}

void inplace_subtraction_result_test()
{
    using namespace hpx::traits::detail;

    struct A
    {
    };
    struct B
    {
    };

    struct C
    {
        B operator-=(A const&) const
        {
            return B{};
        }
    };

    HPX_TEST_MSG((std::is_same_v<B, inplace_subtraction_result_t<C, A>>),
        "deduced type");

    HPX_TEST_MSG((!inplace_subtraction_result<B, A>), "invalid operation");
}

void predecrement_result_test()
{
    using namespace hpx::traits::detail;

    struct A
    {
    };

    struct B
    {
        A& operator--() const
        {
            static A a;
            return a;
        }
    };

    HPX_TEST_MSG(
        (std::is_same_v<A&, predecrement_result_t<B>>), "deduced type");

    HPX_TEST_MSG((!predecrement_result<A>), "invalid operation");
}

void preincrement_result_test()
{
    using namespace hpx::traits::detail;

    struct A
    {
    };

    struct B
    {
        A& operator++() const
        {
            static A a;
            return a;
        }
    };

    HPX_TEST_MSG(
        (std::is_same_v<A&, preincrement_result_t<B>>), "deduced type");

    HPX_TEST_MSG((!preincrement_result<A>), "invalid operation");
}

void postdecrement_result_test()
{
    using namespace hpx::traits::detail;

    struct A
    {
    };

    struct B
    {
        A operator--(int) const
        {
            return A{};
        }
    };

    HPX_TEST_MSG(
        (std::is_same_v<A, postdecrement_result_t<B>>), "deduced type");

    HPX_TEST_MSG((!postdecrement_result<A>), "invalid operation");
}

void postincrement_result_test()
{
    using namespace hpx::traits::detail;

    struct A
    {
    };

    struct B
    {
        A operator++(int) const
        {
            return A{};
        }
    };

    HPX_TEST_MSG(
        (std::is_same_v<A, postincrement_result_t<B>>), "deduced type");

    HPX_TEST_MSG((!postincrement_result<A>), "invalid operation");
}

void subscript_result_test()
{
    using namespace hpx::traits::detail;

    struct A
    {
    };
    struct B
    {
    };

    struct C
    {
        B operator[](A const&) const
        {
            return B{};
        }
    };

    HPX_TEST_MSG((std::is_same_v<B, subscript_result_t<C, A>>), "deduced type");

    HPX_TEST_MSG((!subscript_result<B, A>), "invalid operation");
}

void subtraction_result_test()
{
    using namespace hpx::traits::detail;

    struct A
    {
    };
    struct B
    {
    };

    struct C
    {
        B operator-(A const&) const
        {
            return B{};
        }
    };

    HPX_TEST_MSG(
        (std::is_same_v<B, subtraction_result_t<C, A>>), "deduced type");

    HPX_TEST_MSG((!subtraction_result<B, A>), "invalid operation");
}

void bidirectional_concept_test()
{
    {
        using iterator = std::ostream_iterator<int>;
        HPX_TEST_MSG(
            (!std::bidirectional_iterator<iterator>), "output iterator");
    }
    {
        using iterator = std::istream_iterator<int>;
        HPX_TEST_MSG(
            (!std::bidirectional_iterator<iterator>), "input iterator");
    }
    {
        using iterator = typename std::forward_list<int>::iterator;
        HPX_TEST_MSG(
            (!std::bidirectional_iterator<iterator>), "forward iterator");
    }
    {
        using iterator = typename std::list<int>::iterator;
        HPX_TEST_MSG(
            (std::bidirectional_iterator<iterator>), "bidirectional iterator");
    }
    {
        using iterator = typename std::vector<int>::iterator;
        HPX_TEST_MSG(
            (std::bidirectional_iterator<iterator>), "random access iterator");
    }
    {
        using iterator = bidirectional_traversal_iterator;
        HPX_TEST_MSG((std::bidirectional_iterator<iterator>),
            "bidirectional traversal input iterator");
    }
    {
        using iterator = random_access_traversal_iterator;
        HPX_TEST_MSG((std::bidirectional_iterator<iterator>),
            "random access traversal input iterator");
    }
}

void random_access_concept_test()
{
    using namespace hpx::traits;
    using namespace hpx::traits::detail;

    {
        using iterator = std::ostream_iterator<int>;
        HPX_TEST_MSG(
            (!std::random_access_iterator<iterator>), "output iterator");
    }
    {
        using iterator = std::istream_iterator<int>;
        HPX_TEST_MSG(
            (!std::random_access_iterator<iterator>), "input iterator");
    }
    {
        using iterator = typename std::forward_list<int>::iterator;
        HPX_TEST_MSG(
            (!std::random_access_iterator<iterator>), "forward iterator");
    }
    {
        using iterator = typename std::list<int>::iterator;
        HPX_TEST_MSG(
            (!std::random_access_iterator<iterator>), "bidirectional iterator");
    }
    {
        using iterator = typename std::vector<int>::iterator;
        HPX_TEST_MSG(
            (std::random_access_iterator<iterator>), "random access iterator");
    }
    {
        using iterator = bidirectional_traversal_iterator;
        HPX_TEST_MSG((!std::random_access_iterator<iterator>),
            "bidirectional traversal input iterator");
    }
    {
        using iterator = random_access_traversal_iterator;

        using namespace hpx::traits::detail;
        static_assert(std::is_same_v<iterator,
            addition_result_t<iterator, iter_difference_t<iterator>>>);
        static_assert(std::is_same_v<
            typename std::add_lvalue_reference<iterator>::type,
            inplace_addition_result_t<iterator, iter_difference_t<iterator>>>);
        static_assert(std::is_same_v<iterator,
            subtraction_result_t<iterator, iter_difference_t<iterator>>>);
        static_assert(std::is_same_v<iter_difference_t<iterator>,
            subtraction_result_t<iterator, iterator>>);
        static_assert(std::is_same_v<std::add_lvalue_reference_t<iterator>,
            inplace_subtraction_result_t<iterator,
                iter_difference_t<iterator>>>);

        HPX_TEST_MSG((std::random_access_iterator<iterator>),
            "random access traversal input iterator");
    }
}

void satisfy_traversal_concept_forward_test()
{
    using namespace hpx::traits::detail;

    {
        using iterator = std::ostream_iterator<int>;
        HPX_TEST_MSG((!satisfy_traversal_concept<iterator,
                         hpx::forward_traversal_tag>::value),
            "output iterator");
    }
    {
        using iterator = std::istream_iterator<int>;
        HPX_TEST_MSG((!satisfy_traversal_concept<iterator,
                         hpx::forward_traversal_tag>::value),
            "input iterator");
    }
    {
        // see comment on definition for explanation
        using iterator = typename std::forward_list<int>::iterator;
        HPX_TEST_MSG((!satisfy_traversal_concept<iterator,
                         hpx::forward_traversal_tag>::value),
            "forward iterator");
    }
    {
        using iterator = typename std::list<int>::iterator;
        HPX_TEST_MSG((satisfy_traversal_concept<iterator,
                         hpx::forward_traversal_tag>::value),
            "bidirectional iterator");
    }
    {
        using iterator = typename std::vector<int>::iterator;
        HPX_TEST_MSG((satisfy_traversal_concept<iterator,
                         hpx::forward_traversal_tag>::value),
            "random access iterator");
    }
    {
        using iterator = bidirectional_traversal_iterator;
        HPX_TEST_MSG((satisfy_traversal_concept<iterator,
                         hpx::forward_traversal_tag>::value),
            "bidirectional traversal input iterator");
    }
    {
        using iterator = random_access_traversal_iterator;
        HPX_TEST_MSG((satisfy_traversal_concept<iterator,
                         hpx::forward_traversal_tag>::value),
            "random access traversal input iterator");
    }
}

void satisfy_traversal_concept_bidirectional_test()
{
    using namespace hpx::traits::detail;

    {
        using iterator = std::ostream_iterator<int>;
        HPX_TEST_MSG((!satisfy_traversal_concept<iterator,
                         hpx::bidirectional_traversal_tag>::value),
            "output iterator");
    }
    {
        using iterator = std::istream_iterator<int>;
        HPX_TEST_MSG((!satisfy_traversal_concept<iterator,
                         hpx::bidirectional_traversal_tag>::value),
            "input iterator");
    }
    {
        using iterator = typename std::forward_list<int>::iterator;
        HPX_TEST_MSG((!satisfy_traversal_concept<iterator,
                         hpx::bidirectional_traversal_tag>::value),
            "forward iterator");
    }
    {
        using iterator = typename std::list<int>::iterator;
        HPX_TEST_MSG((satisfy_traversal_concept<iterator,
                         hpx::bidirectional_traversal_tag>::value),
            "bidirectional iterator");
    }
    {
        using iterator = typename std::vector<int>::iterator;
        HPX_TEST_MSG((satisfy_traversal_concept<iterator,
                         hpx::bidirectional_traversal_tag>::value),
            "random access iterator");
    }
    {
        using iterator = bidirectional_traversal_iterator;
        HPX_TEST_MSG((satisfy_traversal_concept<iterator,
                         hpx::bidirectional_traversal_tag>::value),
            "bidirectional traversal input iterator");
    }
    {
        using iterator = random_access_traversal_iterator;
        HPX_TEST_MSG((satisfy_traversal_concept<iterator,
                         hpx::bidirectional_traversal_tag>::value),
            "random access traversal input iterator");
    }
}

void satisfy_traversal_concept_random_access_test()
{
    using namespace hpx::traits::detail;

    {
        using iterator = std::ostream_iterator<int>;
        HPX_TEST_MSG((!satisfy_traversal_concept<iterator,
                         hpx::random_access_traversal_tag>::value),
            "output iterator");
    }
    {
        using iterator = std::istream_iterator<int>;
        HPX_TEST_MSG((!satisfy_traversal_concept<iterator,
                         hpx::random_access_traversal_tag>::value),
            "input iterator");
    }
    {
        using iterator = typename std::forward_list<int>::iterator;
        HPX_TEST_MSG((!satisfy_traversal_concept<iterator,
                         hpx::random_access_traversal_tag>::value),
            "forward iterator");
    }
    {
        using iterator = typename std::list<int>::iterator;
        HPX_TEST_MSG((!satisfy_traversal_concept<iterator,
                         hpx::random_access_traversal_tag>::value),
            "bidirectional iterator");
    }
    {
        using iterator = typename std::vector<int>::iterator;
        HPX_TEST_MSG((satisfy_traversal_concept<iterator,
                         hpx::random_access_traversal_tag>::value),
            "random access iterator");
    }
    {
        using iterator = bidirectional_traversal_iterator;
        HPX_TEST_MSG((!satisfy_traversal_concept<iterator,
                         hpx::random_access_traversal_tag>::value),
            "bidirectional traversal input iterator");
    }
    {
        using iterator = random_access_traversal_iterator;
        HPX_TEST_MSG((satisfy_traversal_concept<iterator,
                         hpx::random_access_traversal_tag>::value),
            "random access traversal input iterator");
    }
}

void is_iterator_test()
{
    using hpx::traits::is_iterator;

    {
        using iterator = std::ostream_iterator<int>;
        HPX_TEST_MSG((is_iterator<iterator>::value), "output iterator");
    }
    {
        using iterator = std::istream_iterator<int>;
        HPX_TEST_MSG((is_iterator<iterator>::value), "input iterator");
    }
    {
        using iterator = typename std::forward_list<int>::iterator;
        HPX_TEST_MSG((is_iterator<iterator>::value), "forward iterator");
    }
    {
        using iterator = typename std::list<int>::iterator;
        HPX_TEST_MSG((is_iterator<iterator>::value), "bidirectional iterator");
    }
    {
        using iterator = typename std::vector<int>::iterator;
        HPX_TEST_MSG((is_iterator<iterator>::value), "random access iterator");
    }
    {
        using iterator = bidirectional_traversal_iterator;
        HPX_TEST_MSG((is_iterator<iterator>::value),
            "bidirectional traversal input iterator");
    }
    {
        using iterator = random_access_traversal_iterator;
        HPX_TEST_MSG((is_iterator<iterator>::value),
            "random access traversal input iterator");
    }
}

void is_output_iterator_test()
{
    using hpx::traits::is_output_iterator;

    // Non-iterator types must yield false without a substitution error.
    static_assert(!hpx::traits::is_output_iterator_v<int>);
    static_assert(!hpx::traits::is_output_iterator_v<std::vector<int>>);

    {
        using iterator = std::ostream_iterator<int>;
        HPX_TEST_MSG((is_output_iterator<iterator>::value), "output iterator");
    }
    {
        using iterator = std::istream_iterator<int>;
        HPX_TEST_MSG((!is_output_iterator<iterator>::value), "input iterator");
    }
    {
        using iterator = typename std::forward_list<int>::iterator;
        HPX_TEST_MSG((is_output_iterator<iterator>::value), "forward iterator");
    }
    {
        using iterator = typename std::list<int>::iterator;
        HPX_TEST_MSG(
            (is_output_iterator<iterator>::value), "bidirectional iterator");
    }
    {
        using iterator = typename std::vector<int>::iterator;
        HPX_TEST_MSG(
            (is_output_iterator<iterator>::value), "random access iterator");
    }
    {
        using iterator = bidirectional_traversal_iterator;
        HPX_TEST_MSG((is_output_iterator<iterator>::value),
            "bidirectional traversal input iterator");
    }
    {
        using iterator = random_access_traversal_iterator;
        HPX_TEST_MSG((is_output_iterator<iterator>::value),
            "random access traversal input iterator");
    }
}

void is_input_iterator_test()
{
    using hpx::traits::is_input_iterator;

    {
        using iterator = std::ostream_iterator<int>;
        HPX_TEST_MSG((!is_input_iterator<iterator>::value), "output iterator");
    }
    {
        using iterator = std::istream_iterator<int>;
        HPX_TEST_MSG((is_input_iterator<iterator>::value), "input iterator");
    }
    {
        using iterator = typename std::forward_list<int>::iterator;
        HPX_TEST_MSG((is_input_iterator<iterator>::value), "forward iterator");
    }
    {
        using iterator = typename std::list<int>::iterator;
        HPX_TEST_MSG(
            (is_input_iterator<iterator>::value), "bidirectional iterator");
    }
    {
        using iterator = typename std::vector<int>::iterator;
        HPX_TEST_MSG(
            (is_input_iterator<iterator>::value), "random access iterator");
    }
    {
        using iterator = bidirectional_traversal_iterator;
        HPX_TEST_MSG((is_input_iterator<iterator>::value),
            "bidirectional traversal input iterator");
    }
    {
        using iterator = random_access_traversal_iterator;
        HPX_TEST_MSG((is_input_iterator<iterator>::value),
            "random access traversal input iterator");
    }
    {
        using base_iterator = std::vector<std::size_t>::iterator;
        using iterator =
            test::test_iterator<base_iterator, std::random_access_iterator_tag>;

        HPX_TEST_MSG((is_input_iterator<iterator>::value), "hpx test iterator");
    }
    {
        using base_iterator = std::vector<std::size_t>::iterator;
        using iterator =
            test::test_iterator<base_iterator, std::bidirectional_iterator_tag>;

        HPX_TEST_MSG((is_input_iterator<iterator>::value), "hpx test iterator");
    }
    {
        using base_iterator = std::vector<std::size_t>::iterator;
        using iterator =
            test::test_iterator<base_iterator, std::forward_iterator_tag>;

        HPX_TEST_MSG((is_input_iterator<iterator>::value), "hpx test iterator");
    }
    {
        using base_iterator = std::vector<std::size_t>::iterator;
        using iterator =
            test::test_iterator<base_iterator, std::input_iterator_tag>;

        HPX_TEST_MSG((is_input_iterator<iterator>::value), "hpx test iterator");
    }
}

void is_forward_iterator_test()
{
    using hpx::traits::is_forward_iterator;

    {
        using iterator = std::ostream_iterator<int>;
        HPX_TEST_MSG(
            (!is_forward_iterator<iterator>::value), "output iterator");
    }
    {
        using iterator = std::istream_iterator<int>;
        HPX_TEST_MSG((!is_forward_iterator<iterator>::value), "input iterator");
    }
    {
        using iterator = typename std::forward_list<int>::iterator;
        HPX_TEST_MSG(
            (is_forward_iterator<iterator>::value), "forward iterator");
    }
    {
        using iterator = typename std::list<int>::iterator;
        HPX_TEST_MSG(
            (is_forward_iterator<iterator>::value), "bidirectional iterator");
    }
    {
        using iterator = typename std::vector<int>::iterator;
        HPX_TEST_MSG(
            (is_forward_iterator<iterator>::value), "random access iterator");
    }
    {
        using iterator = bidirectional_traversal_iterator;
        HPX_TEST_MSG((is_forward_iterator<iterator>::value),
            "bidirectional traversal input iterator");
    }
    {
        using iterator = random_access_traversal_iterator;
        HPX_TEST_MSG((is_forward_iterator<iterator>::value),
            "random access traversal input iterator");
    }
    {
        using base_iterator = std::vector<std::size_t>::iterator;
        using iterator =
            test::test_iterator<base_iterator, std::random_access_iterator_tag>;

        HPX_TEST_MSG(
            (is_forward_iterator<iterator>::value), "hpx test iterator");
    }
    {
        using base_iterator = std::vector<std::size_t>::iterator;
        using iterator =
            test::test_iterator<base_iterator, std::bidirectional_iterator_tag>;

        HPX_TEST_MSG(
            (is_forward_iterator<iterator>::value), "hpx test iterator");
    }
    {
        using base_iterator = std::vector<std::size_t>::iterator;
        using iterator =
            test::test_iterator<base_iterator, std::forward_iterator_tag>;

        HPX_TEST_MSG(
            (is_forward_iterator<iterator>::value), "hpx test iterator");
    }
    {
        using base_iterator = std::vector<std::size_t>::iterator;
        using iterator =
            test::test_iterator<base_iterator, std::input_iterator_tag>;

        HPX_TEST_MSG(
            (!is_forward_iterator<iterator>::value), "hpx test iterator");
    }
}

void is_bidirectional_iterator_test()
{
    using hpx::traits::is_bidirectional_iterator;

    {
        using iterator = std::ostream_iterator<int>;
        HPX_TEST_MSG(
            (!is_bidirectional_iterator<iterator>::value), "output iterator");
    }
    {
        using iterator = std::istream_iterator<int>;
        HPX_TEST_MSG(
            (!is_bidirectional_iterator<iterator>::value), "input iterator");
    }
    {
        using iterator = typename std::forward_list<int>::iterator;
        HPX_TEST_MSG(
            (!is_bidirectional_iterator<iterator>::value), "forward iterator");
    }
    {
        using iterator = typename std::list<int>::iterator;
        HPX_TEST_MSG((is_bidirectional_iterator<iterator>::value),
            "bidirectional iterator");
    }
    {
        using iterator = typename std::vector<int>::iterator;
        HPX_TEST_MSG((is_bidirectional_iterator<iterator>::value),
            "random access iterator");
    }
    {
        using iterator = bidirectional_traversal_iterator;
        HPX_TEST_MSG((is_bidirectional_iterator<iterator>::value),
            "bidirectional traversal input iterator");
    }
    {
        using iterator = random_access_traversal_iterator;
        HPX_TEST_MSG((is_bidirectional_iterator<iterator>::value),
            "random access traversal input iterator");
    }
    {
        using base_iterator = std::vector<std::size_t>::iterator;
        using iterator =
            test::test_iterator<base_iterator, std::random_access_iterator_tag>;

        HPX_TEST_MSG(
            (is_bidirectional_iterator<iterator>::value), "hpx test iterator");
    }
    {
        using base_iterator = std::vector<std::size_t>::iterator;
        using iterator =
            test::test_iterator<base_iterator, std::bidirectional_iterator_tag>;

        HPX_TEST_MSG(
            (is_bidirectional_iterator<iterator>::value), "hpx test iterator");
    }
    {
        using base_iterator = std::vector<std::size_t>::iterator;
        using iterator =
            test::test_iterator<base_iterator, std::forward_iterator_tag>;

        HPX_TEST_MSG(
            (!is_bidirectional_iterator<iterator>::value), "hpx test iterator");
    }
    {
        using base_iterator = std::vector<std::size_t>::iterator;
        using iterator =
            test::test_iterator<base_iterator, std::input_iterator_tag>;

        HPX_TEST_MSG(
            (!is_bidirectional_iterator<iterator>::value), "hpx test iterator");
    }
}

void is_random_access_iterator_test()
{
    using hpx::traits::is_random_access_iterator;

    {
        using iterator = std::ostream_iterator<int>;
        HPX_TEST_MSG(
            (!is_random_access_iterator<iterator>::value), "output iterator");
    }
    {
        using iterator = std::istream_iterator<int>;
        HPX_TEST_MSG(
            (!is_random_access_iterator<iterator>::value), "input iterator");
    }
    {
        using iterator = typename std::forward_list<int>::iterator;
        HPX_TEST_MSG(
            (!is_random_access_iterator<iterator>::value), "forward iterator");
    }
    {
        using iterator = typename std::list<int>::iterator;
        HPX_TEST_MSG((!is_random_access_iterator<iterator>::value),
            "bidirectional iterator");
    }
    {
        using iterator = typename std::vector<int>::iterator;
        HPX_TEST_MSG((is_random_access_iterator<iterator>::value),
            "random access iterator");
    }
    {
        using iterator = bidirectional_traversal_iterator;
        HPX_TEST_MSG((!is_random_access_iterator<iterator>::value),
            "bidirectional traversal input iterator");
    }
    {
        using iterator = random_access_traversal_iterator;
        HPX_TEST_MSG((is_random_access_iterator<iterator>::value),
            "random access traversal input iterator");
    }
    {
        using base_iterator = std::vector<std::size_t>::iterator;
        using iterator =
            test::test_iterator<base_iterator, std::random_access_iterator_tag>;

        HPX_TEST_MSG(
            (is_random_access_iterator<iterator>::value), "hpx test iterator");
    }
    {
        using base_iterator = std::vector<std::size_t>::iterator;
        using iterator =
            test::test_iterator<base_iterator, std::bidirectional_iterator_tag>;

        HPX_TEST_MSG(
            (!is_random_access_iterator<iterator>::value), "hpx test iterator");
    }
    {
        using base_iterator = std::vector<std::size_t>::iterator;
        using iterator =
            test::test_iterator<base_iterator, std::forward_iterator_tag>;

        HPX_TEST_MSG(
            (!is_random_access_iterator<iterator>::value), "hpx test iterator");
    }
    {
        using base_iterator = std::vector<std::size_t>::iterator;
        using iterator =
            test::test_iterator<base_iterator, std::input_iterator_tag>;

        HPX_TEST_MSG(
            (!is_random_access_iterator<iterator>::value), "hpx test iterator");
    }
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    {
        addition_result_test();
        dereference_result_test();
        equality_result_test();
        inequality_result_test();
        inplace_addition_result_test();
        inplace_subtraction_result_test();
        predecrement_result_test();
        preincrement_result_test();
        postdecrement_result_test();
        postincrement_result_test();
        subscript_result_test();
        subtraction_result_test();
        bidirectional_concept_test();
        random_access_concept_test();
        satisfy_traversal_concept_forward_test();
        satisfy_traversal_concept_bidirectional_test();
        satisfy_traversal_concept_random_access_test();
        is_iterator_test();
        is_input_iterator_test();
        is_output_iterator_test();
        is_forward_iterator_test();
        is_bidirectional_iterator_test();
        is_random_access_iterator_test();
    }

    return hpx::util::report_errors();
}
