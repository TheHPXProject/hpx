//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

void test_concurrent_vector()
{
    hpx::concurrent::concurrent_vector<int> v;
    std::atomic<int> count{0};

    std::vector<hpx::thread> threads;
    threads.reserve(10);
    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back([&v, &count, i] {
            for (int j = 0; j < 100; ++j)
            {
                v.push_back(i * 100 + j);
                count++;
            }
        });
    }

    for (auto& t : threads)
        t.join();

    HPX_TEST_EQ(static_cast<int>(v.size()), 1000);
    HPX_TEST_EQ(count.load(), 1000);

    // Test accessor
    if (v.size() > 0)
    {
        v[0].set(999);
        HPX_TEST_EQ(static_cast<int>(v[0].get()), 999);
        HPX_TEST(v[0].get() == 999);

        // Test implicit conversion to reference
        {
            auto&& acc = v[0];
            int& ref = acc;
            ref = 123;
        }
        HPX_TEST_EQ(static_cast<int>(v[0].get()), 123);
    }

    // Test at()
    HPX_TEST_EQ(static_cast<int>(v.at(0).get()), 123);
}

void test_concurrent_vector_reserve()
{
    hpx::concurrent::concurrent_vector<int> v;
    v.reserve(500);
    HPX_TEST(v.capacity() >= 500);

    for (int i = 0; i < 500; ++i)
        v.push_back(i);
    HPX_TEST_EQ(static_cast<int>(v.size()), 500);
}

void test_concurrent_vector_grow_by()
{
    hpx::concurrent::concurrent_vector<int> v;
    auto old_size = v.grow_by(10);
    HPX_TEST_EQ(old_size, 0u);
    HPX_TEST_EQ(v.size(), 10u);

    old_size = v.grow_by(5, 42);
    HPX_TEST_EQ(old_size, 10u);
    HPX_TEST_EQ(v.size(), 15u);
    HPX_TEST_EQ(static_cast<int>(v[10].get()), 42);
}

void test_concurrent_vector_for_each()
{
    hpx::concurrent::concurrent_vector<int> v;
    for (int i = 0; i < 100; ++i)
        v.push_back(i);

    std::atomic<int> sum{0};
    v.for_each([&sum](int val) { sum += val; });
    HPX_TEST_EQ(sum.load(), 4950);

    std::atomic<int> count{0};
    v.for_each([&count](int) {
        count++;
        return count.load() < 50;
    });
    HPX_TEST_EQ(count.load(), 50);
}

void test_concurrent_unordered_map()
{
    hpx::concurrent::concurrent_unordered_map<int, int> m;
    std::atomic<int> count{0};

    std::vector<hpx::thread> threads;
    threads.reserve(10);
    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back([&m, &count, i] {
            for (int j = 0; j < 100; ++j)
            {
                m.insert({i * 100 + j, j});
                count++;
            }
        });
    }

    for (auto& t : threads)
        t.join();

    HPX_TEST_EQ(static_cast<int>(m.size()), 1000);
    HPX_TEST_EQ(count.load(), 1000);

    // Test concurrent read/write
    threads.clear();
    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back([&m, i] {
            for (int j = 0; j < 100; ++j)
            {
                m[i * 100 + j].set(j + 1);         // Update
                int val = m[i * 100 + j].get();    // Read
                HPX_TEST_EQ(val, j + 1);

                // Test implicit conversion to reference
                {
                    auto&& acc = m[i * 100 + j];
                    int& ref = acc;
                    ref = j + 2;
                }
                HPX_TEST_EQ(static_cast<int>(m[i * 100 + j].get()), j + 2);
            }
        });
    }

    for (auto& t : threads)
        t.join();

    std::atomic<int> sum{0};
    m.for_each([&sum](auto const& kv) { sum += kv.second; });
    // 10 threads * sum(2..101) = 10 * 5150 = 51500
    HPX_TEST_EQ(sum.load(), 51500);

    // Test erase
    HPX_TEST_EQ(m.erase(0), 1u);
    HPX_TEST_EQ(m.erase(0), 0u);
    HPX_TEST_EQ(m.size(), 999u);
}

void test_concurrent_unordered_map_extra()
{
    // Test constructors
    hpx::concurrent::concurrent_unordered_map<int, int> m1(100);
    HPX_TEST(m1.bucket_count() >= 100);

    hpx::concurrent::concurrent_unordered_map<std::string, int> m_str;
    std::string key = "hello";
    m_str.insert({key, 42});

    {
        auto acc = m_str.find(key);
        HPX_TEST(!acc.empty());
        if (acc)
        {
            HPX_TEST_EQ(acc.get(), 42);
        }
    }

    HPX_TEST(m_str.contains(key));

    m_str[key].set(43);
    HPX_TEST_EQ(m_str[key].get(), 43);

    {
        struct transparent_hash
        {
            using is_transparent = void;
            std::size_t operator()(std::string_view sv) const
            {
                return std::hash<std::string_view>{}(sv);
            }
            std::size_t operator()(std::string const& s) const
            {
                return std::hash<std::string>{}(s);
            }
        };

        struct transparent_equal
        {
            using is_transparent = void;
            bool operator()(std::string_view sv, std::string const& s) const
            {
                return sv == s;
            }
            bool operator()(std::string const& s, std::string_view sv) const
            {
                return s == sv;
            }
            bool operator()(std::string const& s1, std::string const& s2) const
            {
                return s1 == s2;
            }
        };

        hpx::concurrent::concurrent_unordered_map<std::string, int,
            transparent_hash, transparent_equal>
            mt;
        mt.insert({"hello", 42});

        std::string_view sv = "hello";
        {
            auto acc = mt.find(sv);
            HPX_TEST(!acc.empty());
            HPX_TEST_EQ(acc.get(), 42);
        }
        HPX_TEST(mt.contains(sv));
        HPX_TEST_EQ(mt.count(sv), 1u);

#if defined(HPX_HAVE_CXX23_STD_UNORDERED_TRANSPARENT_ERASE)
        HPX_TEST_EQ(mt.erase(sv), 1u);
        HPX_TEST(mt.empty());
#endif

#if defined(HPX_HAVE_CXX26_STD_UNORDERED_TRANSPARENT_LOOKUP)
        mt["world"] = 88;
        HPX_TEST_EQ(mt.at(std::string_view("world")).get(), 88);
#endif
    }
}

void test_concurrent_unordered_set()
{
    hpx::concurrent::concurrent_unordered_set<int> s;
    std::atomic<int> count{0};

    std::vector<hpx::thread> threads;
    threads.reserve(10);
    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back([&s, &count, i] {
            for (int j = 0; j < 100; ++j)
            {
                s.insert(i * 100 + j);
                count++;
            }
        });
    }

    for (auto& t : threads)
        t.join();

    HPX_TEST_EQ(static_cast<int>(s.size()), 1000);
    HPX_TEST_EQ(count.load(), 1000);

    std::atomic<int> set_count{0};
    s.for_each([&set_count](auto const&) { set_count++; });
    HPX_TEST_EQ(set_count.load(), 1000);

    // Test erase
    HPX_TEST_EQ(s.erase(0), 1u);
    HPX_TEST_EQ(s.erase(0), 0u);
    HPX_TEST_EQ(s.size(), 999u);
}

void test_concurrent_unordered_set_extra()
{
    hpx::concurrent::concurrent_unordered_set<std::string> s;
    std::string key = "world";
    s.insert(key);

    HPX_TEST(s.contains(key));

    {
        auto acc = s.find(key);
        HPX_TEST(!acc.empty());
        if (acc)
        {
            HPX_TEST_EQ(acc.get(), "world");
        }
    }

    {
        struct transparent_hash
        {
            using is_transparent = void;
            std::size_t operator()(std::string_view sv) const
            {
                return std::hash<std::string_view>{}(sv);
            }
            std::size_t operator()(std::string const& s) const
            {
                return std::hash<std::string>{}(s);
            }
        };

        struct transparent_equal
        {
            using is_transparent = void;
            bool operator()(std::string_view sv, std::string const& s) const
            {
                return sv == s;
            }
            bool operator()(std::string const& s, std::string_view sv) const
            {
                return s == sv;
            }
            bool operator()(std::string const& s1, std::string const& s2) const
            {
                return s1 == s2;
            }
        };

        hpx::concurrent::concurrent_unordered_set<std::string, transparent_hash,
            transparent_equal>
            st;
        st.insert("world");

        std::string_view sv = "world";
        {
            auto acc = st.find(sv);
            HPX_TEST(!acc.empty());
        }
        HPX_TEST(st.contains(sv));
        HPX_TEST_EQ(st.count(sv), 1u);

#if defined(HPX_HAVE_CXX23_STD_UNORDERED_TRANSPARENT_ERASE)
        HPX_TEST_EQ(st.erase(sv), 1u);
        HPX_TEST(st.empty());
#endif
    }
}

void test_concurrent_queue()
{
    hpx::concurrent::concurrent_queue<int> q100(100);
    std::atomic<int> count{0};

    std::vector<hpx::thread> threads;
    threads.reserve(10);
    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back([&q100, &count, i] {
            for (int j = 0; j < 100; ++j)
            {
                q100.push(i * 100 + j);
                count++;
            }
        });
    }

    for (auto& t : threads)
        t.join();

    HPX_TEST_EQ(count.load(), 1000);

    int val;
    int popped_count = 0;
    while (q100.pop(val))
    {
        popped_count++;
    }

    HPX_TEST_EQ(popped_count, 1000);
}

void test_concurrent_unordered_map_for_each_break()
{
    hpx::concurrent::concurrent_unordered_map<int, int> m;
    for (int i = 0; i < 100; ++i)
        m.insert({i, i});

    std::atomic<int> count{0};
    m.for_each([&count](auto const&) {
        count++;
        return count.load() < 50;
    });

    HPX_TEST_EQ(count.load(), 50);
}

void test_concurrent_unordered_set_for_each_break()
{
    hpx::concurrent::concurrent_unordered_set<int> s;
    for (int i = 0; i < 100; ++i)
        s.insert(i);

    std::atomic<int> count{0};
    s.for_each([&count](auto const&) {
        count++;
        return count.load() < 50;
    });

    HPX_TEST_EQ(count.load(), 50);
}

void test_move_assignment()
{
    // concurrent_unordered_map
    {
        hpx::concurrent::concurrent_unordered_map<int, int> m1;
        m1.insert({1, 10});
        m1.insert({2, 20});
        m1.insert({3, 30});

        hpx::concurrent::concurrent_unordered_map<int, int> m2;
        m2 = std::move(m1);

        HPX_TEST_EQ(m2.size(), 3u);
        HPX_TEST_EQ(static_cast<int>(m2.at(1).get()), 10);
        HPX_TEST_EQ(static_cast<int>(m2.at(2).get()), 20);
        HPX_TEST_EQ(static_cast<int>(m2.at(3).get()), 30);

        m1.clear();    // NOLINT(bugprone-use-after-move)
        m1.insert({4, 40});
        HPX_TEST_EQ(m1.size(), 1u);
        HPX_TEST_EQ(static_cast<int>(m1.at(4).get()), 40);
    }

    // concurrent_unordered_set
    {
        hpx::concurrent::concurrent_unordered_set<int> s1;
        s1.insert(10);
        s1.insert(20);
        s1.insert(30);

        hpx::concurrent::concurrent_unordered_set<int> s2;
        s2 = std::move(s1);

        HPX_TEST_EQ(s2.size(), 3u);
        HPX_TEST_EQ(s2.count(10), 1u);
        HPX_TEST_EQ(s2.count(20), 1u);
        HPX_TEST_EQ(s2.count(30), 1u);

        s1.clear();    // NOLINT(bugprone-use-after-move)
        s1.insert(40);
        HPX_TEST_EQ(s1.size(), 1u);
        HPX_TEST_EQ(s1.count(40), 1u);
    }

    // concurrent_vector
    {
        hpx::concurrent::concurrent_vector<int> v1;
        v1.push_back(100);
        v1.push_back(200);
        v1.push_back(300);

        hpx::concurrent::concurrent_vector<int> v2;
        v2 = std::move(v1);

        HPX_TEST_EQ(v2.size(), 3u);
        HPX_TEST_EQ(static_cast<int>(v2[0].get()), 100);
        HPX_TEST_EQ(static_cast<int>(v2[1].get()), 200);
        HPX_TEST_EQ(static_cast<int>(v2[2].get()), 300);

        v1.clear();    // NOLINT(bugprone-use-after-move)
        v1.push_back(400);
        HPX_TEST_EQ(v1.size(), 1u);
        HPX_TEST_EQ(static_cast<int>(v1[0].get()), 400);
    }
}

int hpx_main(hpx::program_options::variables_map&)
{
    test_concurrent_vector();
    test_concurrent_vector_reserve();
    test_concurrent_vector_grow_by();
    test_concurrent_vector_for_each();

    test_concurrent_unordered_map();
    test_concurrent_unordered_map_extra();
    test_concurrent_unordered_map_for_each_break();

    test_concurrent_unordered_set();
    test_concurrent_unordered_set_extra();
    test_concurrent_unordered_set_for_each_break();

    test_concurrent_queue();
    test_move_assignment();

    std::cout << "All concurrent data structure tests PASSED!" << std::endl;

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    hpx::init_params init_args;
    return hpx::init(hpx_main, argc, argv, init_args);
}
