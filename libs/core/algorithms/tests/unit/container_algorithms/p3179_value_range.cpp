//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <concepts>
#include <memory>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
    template <typename T>
    concept can_unwrap = requires(T&& value) {
        hpx::parallel::detail::unwrap_algorithm_value(HPX_FORWARD(T, value));
    };

    using stored_value = hpx::parallel::detail::algorithm_value<int>;
    static_assert(can_unwrap<int&> && can_unwrap<int const&>);
    static_assert(can_unwrap<stored_value&> && can_unwrap<stored_value const&>);
    static_assert(!can_unwrap<int> && !can_unwrap<int const>);
    static_assert(!can_unwrap<stored_value> && !can_unwrap<stored_value const>);
    static_assert(
        std::same_as<decltype(hpx::parallel::detail::unwrap_algorithm_value(
                         std::declval<stored_value&>())),
            int const&>);

    template <typename T>
    auto value(T&& result)
    {
        if constexpr (hpx::traits::is_future_v<std::decay_t<T>>)
            return result.get();
        else
            return result;
    }

    struct token
    {
        int key = 0;

        token() = default;

        explicit token(int key)
          : key(key)
        {
        }

        token(token const&) = delete;
        token& operator=(token const&) = default;
        token(token&&) = default;
        token& operator=(token&&) = default;

        friend bool operator==(token const&, token const&) = default;
    };

    // Copyable destination, assignable and constructible from a noncopyable
    // source. Its projection also supplies a noncopyable comparison value.
    struct record
    {
        std::shared_ptr<token> data;

        record()
          : data(std::make_shared<token>(0))
        {
        }

        explicit record(token const& t)
          : data(std::make_shared<token>(t.key))
        {
        }

        record& operator=(token const& t)
        {
            data = std::make_shared<token>(t.key);
            return *this;
        }
    };

    struct projection
    {
        token const& operator()(record const& r) const
        {
            return *r.data;
        }
    };

    using iterator = std::vector<record>::iterator;
    static_assert(std::indirect_binary_predicate<std::ranges::equal_to,
        std::projected<iterator, projection>, token const*>);
    static_assert(std::indirectly_writable<iterator, token const&>);
    static_assert(std::constructible_from<record, token const&>);
    static_assert(!std::copy_constructible<token>);

    struct copyable_token : token
    {
        using token::token;

        copyable_token(copyable_token const& other)
          : token(other.key)
        {
        }
    };

    struct matches_token
    {
        bool operator()(token const& value) const
        {
            return value.key == 1;
        }
    };

    template <typename Policy, typename Value, bool Accepted>
    constexpr void check_value_category()
    {
        using namespace hpx::ranges;
        using range = std::vector<record>&;
        using iter = iterator;
        using size = std::iter_difference_t<iter>;
        using proj = projection;
        using pred = matches_token;
        using named = token const&;

        static_assert(
            std::is_constructible_v<hpx::parallel::detail::algorithm_value<
                                        std::remove_cvref_t<Value>>,
                Value> == Accepted);
        static_assert(requires(Value&& v) {
            hpx::parallel::detail::equal_to_value(HPX_FORWARD(Value, v));
        } == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::find), Policy,
                          iter, iter, Value, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::find), Policy,
                          range, Value, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::find_last),
                          Policy, iter, iter, Value, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::find_last),
                          Policy, range, Value, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::count), Policy,
                          iter, iter, Value, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::count), Policy,
                          range, Value, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::contains),
                          Policy, iter, iter, Value, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::contains),
                          Policy, range, Value, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::remove), Policy,
                          iter, iter, Value, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::remove), Policy,
                          range, Value, proj> == Accepted);
        static_assert(
            std::is_invocable_v<decltype(hpx::ranges::search_n), Policy, iter,
                iter, size, Value, std::ranges::equal_to, proj> == Accepted);
        static_assert(
            std::is_invocable_v<decltype(hpx::ranges::search_n), Policy, range,
                size, Value, std::ranges::equal_to, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::fill), Policy,
                          iter, iter, Value> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::fill), Policy,
                          range, Value> == Accepted);
        static_assert(
            std::is_invocable_v<decltype(hpx::ranges::uninitialized_fill),
                Policy, iter, iter, Value> == Accepted);
        static_assert(
            std::is_invocable_v<decltype(hpx::ranges::uninitialized_fill),
                Policy, range, Value> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::fill_n), Policy,
                          iter, size, Value> == Accepted);
        static_assert(
            std::is_invocable_v<decltype(hpx::ranges::uninitialized_fill_n),
                Policy, iter, size, Value> == Accepted);
        static_assert(
            std::is_invocable_v<decltype(hpx::ranges::remove_copy), Policy,
                iter, iter, iter, iter, Value, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::remove_copy),
                          Policy, range, range, Value, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::replace_if),
                          Policy, iter, iter, pred, Value, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::replace_if),
                          Policy, range, pred, Value, proj> == Accepted);
        static_assert(
            std::is_invocable_v<decltype(hpx::ranges::replace_copy_if), Policy,
                iter, iter, iter, iter, pred, Value, proj> == Accepted);
        static_assert(
            std::is_invocable_v<decltype(hpx::ranges::replace_copy_if), Policy,
                range, range, pred, Value, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::replace),
                          Policy, iter, iter, Value, named, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::replace),
                          Policy, iter, iter, named, Value, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::replace),
                          Policy, iter, iter, Value, Value, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::replace),
                          Policy, range, Value, named, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::replace),
                          Policy, range, named, Value, proj> == Accepted);
        static_assert(std::is_invocable_v<decltype(hpx::ranges::replace),
                          Policy, range, Value, Value, proj> == Accepted);
        static_assert(
            std::is_invocable_v<decltype(hpx::ranges::replace_copy), Policy,
                iter, iter, iter, iter, Value, named, proj> == Accepted);
        static_assert(
            std::is_invocable_v<decltype(hpx::ranges::replace_copy), Policy,
                iter, iter, iter, iter, named, Value, proj> == Accepted);
        static_assert(
            std::is_invocable_v<decltype(hpx::ranges::replace_copy), Policy,
                iter, iter, iter, iter, Value, Value, proj> == Accepted);
        static_assert(
            std::is_invocable_v<decltype(hpx::ranges::replace_copy), Policy,
                range, range, Value, named, proj> == Accepted);
        static_assert(
            std::is_invocable_v<decltype(hpx::ranges::replace_copy), Policy,
                range, range, named, Value, proj> == Accepted);
        static_assert(
            std::is_invocable_v<decltype(hpx::ranges::replace_copy), Policy,
                range, range, Value, Value, proj> == Accepted);
    }

    template <typename Policy>
    constexpr void check_value_categories()
    {
        check_value_category<Policy, token&, true>();
        check_value_category<Policy, token const&, true>();
        check_value_category<Policy, token, false>();
        check_value_category<Policy, token const, false>();
        check_value_category<Policy, copyable_token, true>();
        check_value_category<Policy, copyable_token const, true>();

        // Typed overloads for braced values must not bypass the lifetime guard.
        static_assert(!requires(Policy policy, std::vector<record>& input) {
            hpx::ranges::find(policy, input, {}, projection{});
        });
        static_assert(!requires(Policy policy, std::vector<record>& input) {
            hpx::ranges::find_last(policy, input, {}, projection{});
        });
        static_assert(!requires(Policy policy, std::vector<record>& input) {
            hpx::ranges::count(policy, input, {}, projection{});
        });
        static_assert(!requires(Policy policy, std::vector<record>& input) {
            hpx::ranges::contains(policy, input, {}, projection{});
        });
        static_assert(!requires(Policy policy, std::vector<record>& input) {
            hpx::ranges::remove(policy, input, {}, projection{});
        });
        static_assert(!requires(Policy policy, std::vector<record>& input) {
            hpx::ranges::search_n(
                policy, input, 1, {}, std::ranges::equal_to{}, projection{});
        });
        static_assert(!requires(Policy policy, std::vector<record>& input) {
            hpx::ranges::remove_copy(policy, input, input, {}, projection{});
        });
        static_assert(!requires(Policy policy, std::vector<record>& input) {
            hpx::ranges::replace(policy, input, {}, record{}, projection{});
        });
        static_assert(!requires(Policy policy, std::vector<record>& input) {
            hpx::ranges::replace_copy(
                policy, input, input, {}, record{}, projection{});
        });
        static_assert(!requires(Policy policy, std::vector<token>& input) {
            hpx::ranges::fill(policy, input, {});
        });
        static_assert(!requires(Policy policy, std::vector<token>& input) {
            hpx::ranges::fill_n(policy, input.begin(), 1, {});
        });
        static_assert(!requires(Policy policy, std::vector<token>& input) {
            hpx::ranges::replace_if(
                policy, input, [](token const&) { return true; }, {});
        });
        static_assert(!requires(Policy policy, std::vector<token>& input) {
            hpx::ranges::replace_copy_if(
                policy, input, input, [](token const&) { return true; }, {});
        });
    }

    template <typename Policy>
    void test_lookup(Policy policy)
    {
        using namespace hpx::ranges;
        std::vector<std::unique_ptr<int>> input(5);
        input.front() = std::make_unique<int>(1);
        input[3] = std::make_unique<int>(2);
        std::unique_ptr<int> const needle;
        auto const first = input.begin();
        auto const last = input.end();

        HPX_TEST(value(find(policy, input, needle)) == first + 1);
        HPX_TEST(value(find(policy, first, last, needle)) == first + 1);
        HPX_TEST(value(count(policy, input, needle)) == 3);
        HPX_TEST(value(count(policy, first, last, needle)) == 3);
        HPX_TEST(value(contains(policy, input, needle)));
        HPX_TEST(value(contains(policy, first, last, needle)));
        auto tail = value(find_last(policy, input, needle));
        HPX_TEST(tail.begin() == first + 4);
        HPX_TEST(tail.end() == last);
        HPX_TEST(
            value(find_last(policy, first, last, needle)).begin() == first + 4);
        auto run = value(search_n(policy, input, 2, needle));
        HPX_TEST(run.begin() == first + 1);
        HPX_TEST(run.end() == first + 3);
        HPX_TEST(value(search_n(policy, first, last, 2, needle)).begin() ==
            first + 1);
        HPX_TEST(value(search_n(policy, input, 0, needle)).begin() == first);
        HPX_TEST(value(search_n(policy, input, 4, needle)).begin() == last);

        std::vector<std::unique_ptr<int>> empty;
        HPX_TEST(value(find(policy, empty, needle)) == empty.end());
        HPX_TEST(value(count(policy, empty, needle)) == 0);
        HPX_TEST(!value(contains(policy, empty, needle)));
        HPX_TEST(
            value(find_last(policy, empty, needle)).begin() == empty.end());
        HPX_TEST(
            value(search_n(policy, empty, 1, needle)).begin() == empty.end());
        HPX_TEST(value(hpx::ranges::remove(policy, input, needle)).begin() ==
            first + 2);
        HPX_TEST(*input.front() == 1);
        HPX_TEST(*input[1] == 2);
    }

    template <typename Policy>
    void test_values(Policy policy)
    {
        using namespace hpx::ranges;
        token const needle(1);
        token const replacement(9);
        std::vector<record> input(5);
        input[1] = needle;
        input[2] = needle;
        auto const proj = projection{};
        auto const pred = [](token const& t) { return t.key == 1; };
        HPX_TEST(value(find(policy, input, needle, proj)) == input.begin() + 1);
        HPX_TEST(value(count(policy, input, needle, proj)) == 2);
        HPX_TEST(value(contains(policy, input, needle, proj)));
        HPX_TEST(value(find_last(policy, input, needle, proj)).begin() ==
            input.begin() + 2);
        HPX_TEST(value(search_n(policy, input, 2, needle,
                           std::ranges::equal_to{}, proj))
                     .begin() == input.begin() + 1);

        std::vector<record> output(2);
        auto copied = value(remove_copy(policy, input, output, needle, proj));
        HPX_TEST(copied.in == input.begin() + 4);
        HPX_TEST(copied.out == output.end());
        HPX_TEST(output[0].data->key == 0);
        HPX_TEST(output[1].data->key == 0);
        copied = value(
            replace_copy(policy, input, output, needle, replacement, proj));
        HPX_TEST(copied.in == input.begin() + 2);
        HPX_TEST(output[0].data->key == 0);
        HPX_TEST(output[1].data->key == 9);
        copied = value(
            replace_copy_if(policy, input, output, pred, replacement, proj));
        HPX_TEST(copied.out == output.end());
        HPX_TEST(output[1].data->key == 9);

        auto modified = input;
        HPX_TEST(value(replace(policy, modified, needle, replacement, proj)) ==
            modified.end());
        HPX_TEST(modified[1].data->key == 9);
        HPX_TEST(modified[2].data->key == 9);
        modified = input;
        HPX_TEST(value(replace_if(policy, modified, pred, replacement, proj)) ==
            modified.end());
        HPX_TEST(modified[1].data->key == 9);
        HPX_TEST(modified[2].data->key == 9);
        HPX_TEST(
            value(hpx::ranges::remove(policy, input, needle, proj)).begin() ==
            input.begin() + 3);

        HPX_TEST(value(fill(policy, output, replacement)) == output.end());
        HPX_TEST(output[0].data->key == 9);
        HPX_TEST(output[1].data->key == 9);
        HPX_TEST(value(fill_n(policy, output.begin(), 1, needle)) ==
            output.begin() + 1);
        HPX_TEST(output[0].data->key == 1);
        HPX_TEST(output[1].data->key == 9);
        HPX_TEST(value(fill_n(policy, output.begin(), -1, needle)) ==
            output.begin());

        std::allocator<record> alloc;
        auto storage = alloc.allocate(2);
        auto range = std::ranges::subrange(storage, storage + 2);
        HPX_TEST(value(uninitialized_fill(policy, range, replacement)) ==
            storage + 2);
        HPX_TEST(storage[0].data->key == 9);
        HPX_TEST(storage[1].data->key == 9);
        std::ranges::destroy(range);
        HPX_TEST(value(uninitialized_fill_n(policy, storage, 2, needle)) ==
            storage + 2);
        HPX_TEST(storage[0].data->key == 1);
        HPX_TEST(storage[1].data->key == 1);
        std::ranges::destroy(range);
        alloc.deallocate(storage, 2);
    }

    template <typename Policy>
    void test_temporary_value(Policy policy)
    {
        std::vector<std::string> input{
            "first", "a long temporary search value"};
        hpx::promise<void> release;
        auto gate = release.get_future().share();
        auto project = [gate](std::string const& s) -> std::string const& {
            gate.get();
            return s;
        };
        auto result = hpx::ranges::find(policy, input,
            std::string("a long temporary search value"), project);
        auto counted = hpx::ranges::count(policy, input,
            std::string("a long temporary search value"), project);
        auto contained = hpx::ranges::contains(policy, input,
            std::string("a long temporary search value"), project);
        // Force the tasks to use their values after the arguments are destroyed.
        release.set_value();
        HPX_TEST(result.get() == input.begin() + 1);
        HPX_TEST_EQ(counted.get(), 1);
        HPX_TEST(contained.get());
    }
    template <typename Policy>
    void test_borrowed_value(Policy policy)
    {
        token const needle(3);
        std::vector<record> input;
        input.emplace_back(needle);
        hpx::promise<void> release;
        auto gate = release.get_future().share();
        auto project = [gate](record const& r) -> token const& {
            gate.get();
            return *r.data;
        };
        auto result = hpx::ranges::count(policy, input, needle, project);
        release.set_value();
        HPX_TEST_EQ(result.get(), 1);
    }
}    // namespace

int hpx_main()
{
    using namespace hpx::execution;
    check_value_categories<decltype(seq)>();
    check_value_categories<decltype(seq(task))>();
    check_value_categories<decltype(par(task))>();
    test_lookup(seq);
    test_lookup(par);
    test_lookup(par_unseq);
    test_lookup(seq(task));
    test_lookup(par(task));
    test_values(seq);
    test_values(par);
    test_values(par_unseq);
    test_values(seq(task));
    test_values(par(task));
    test_temporary_value(seq(task));
    test_temporary_value(par(task));
    test_borrowed_value(seq(task));
    test_borrowed_value(par(task));
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST(hpx::local::init(hpx_main, argc, argv) == 0);
    return hpx::util::report_errors();
}
