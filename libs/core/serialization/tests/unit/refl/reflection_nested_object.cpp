//  Copyright (c) 2026 Ujjwal Shekhar
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/datastructures/optional.hpp>
#include <hpx/datastructures/serialization/optional.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <array>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <iostream>

class person
{
private:
    int age;
    std::string name;

public:
    person()
      : age(0)
    {
    }
    person(int a, std::string n)
      : age(a)
      , name(std::move(n))
    {
    }

    int get_age() const
    {
        return age;
    }

    std::string get_name() const
    {
        return name;
    }

    bool operator==(person const& rhs) const
    {
        return age == rhs.age && name == rhs.name;
    }

    // for std::map, std::set, etc.
    bool operator<(person const& rhs) const
    {
        if (age != rhs.age)
            return age < rhs.age;
        return name < rhs.name;
    }
};

enum class Color
{
    red,
    black,
    white
};

class complicated_object
{
private:
    Color color;
    int a;
    std::string b;
    std::vector<person> c;
    std::list<std::string> d;
    std::deque<int> e;
    std::map<int, person> f;
    std::multimap<int, person> g;
    std::set<std::string> h;
    std::multiset<int> i;
    std::unordered_map<int, person> j;
    std::unordered_multimap<int, int> k;
    std::array<person, 2> m;
    std::pair<std::string, person> o;
    hpx::optional<int> p;
    std::unique_ptr<int> q;

public:
    complicated_object()
      : color(Color::red)
      , a(0)
    {
    }
    complicated_object(Color col, int a_val, std::string b_val,
        std::vector<person> c_val, std::list<std::string> d_val,
        std::deque<int> e_val, std::map<int, person> f_val,
        std::multimap<int, person> g_val, std::set<std::string> h_val,
        std::multiset<int> i_val, std::unordered_map<int, person> j_val,
        std::unordered_multimap<int, int> k_val, std::array<person, 2> m_val,
        std::pair<std::string, person> o_val, hpx::optional<int> p_val,
        std::unique_ptr<int> q_val)
      : color(col)
      , a(a_val)
      , b(std::move(b_val))
      , c(std::move(c_val))
      , d(std::move(d_val))
      , e(std::move(e_val))
      , f(std::move(f_val))
      , g(std::move(g_val))
      , h(std::move(h_val))
      , i(std::move(i_val))
      , j(std::move(j_val))
      , k(std::move(k_val))
      , m(std::move(m_val))
      , o(std::move(o_val))
      , p(std::move(p_val))
      , q(std::move(q_val))
    {
    }

    bool operator==(complicated_object const& rhs) const
    {
        // unique_ptr must be compared manually
        bool uptr_q_equal = ((!q && !rhs.q) || (q && rhs.q && *q == *rhs.q));

        return color == rhs.color && a == rhs.a && b == rhs.b && c == rhs.c &&
            d == rhs.d && e == rhs.e && f == rhs.f && g == rhs.g &&
            h == rhs.h && i == rhs.i && j == rhs.j && k == rhs.k &&
            m == rhs.m && o == rhs.o && p == rhs.p && uptr_q_equal;
    }

    void print() const
    {
        std::cout << "Color: " << static_cast<int>(color) << ", a: " << a
                  << ", b: " << b << ", c: { ";
        for (auto const& person_obj : c)
        {
            std::cout << "{Age: " << person_obj.get_age()
                      << ", Name: " << person_obj.get_name() << "} ";
        }
        std::cout << "}" << std::endl;
        // print all

        std::cout << "d: { ";
        for (auto const& str : d)
        {
            std::cout << str << " ";
        }
        std::cout << "}" << std::endl;

        std::cout << "e: { ";
        for (auto const& val : e)
        {
            std::cout << val << " ";
        }
        std::cout << "}" << std::endl;

        std::cout << "f: { ";
        for (auto const& [key, person_obj] : f)
        {
            std::cout << "{Key: " << key << ", Age: " << person_obj.get_age()
                      << ", Name: " << person_obj.get_name() << "} ";
        }
        std::cout << "}" << std::endl;

        std::cout << "g: { ";
        for (auto const& [key, person_obj] : g)
        {
            std::cout << "{Key: " << key << ", Age: " << person_obj.get_age()
                      << ", Name: " << person_obj.get_name() << "} ";
        }
        std::cout << "}" << std::endl;

        std::cout << "h: { ";
        for (auto const& str : h)
        {
            std::cout << str << " ";
        }
        std::cout << "}" << std::endl;

        std::cout << "i: { ";
        for (auto const& val : i)
        {
            std::cout << val << " ";
        }
        std::cout << "}" << std::endl;

        std::cout << "j: { ";
        for (auto const& [key, person_obj] : j)
        {
            std::cout << "{Key: " << key << ", Age: " << person_obj.get_age()
                      << ", Name: " << person_obj.get_name() << "} ";
        }
        std::cout << "}" << std::endl;

        std::cout << "k: { ";
        for (auto const& [key, val] : k)
        {
            std::cout << "{Key: " << key << ", Value: " << val << "} ";
        }
        std::cout << "}" << std::endl;

        // std::cout << "m: { ";
        // for (auto const& val : m)
        // {
        //     std::cout << val << " ";
        // }
        // std::cout << "}" << std::endl;
        std::cout << "m: { ";
        for (auto const& person_obj : m)
        {
            std::cout << "{Age: " << person_obj.get_age()
                      << ", Name: " << person_obj.get_name() << "} ";
        }
        std::cout << "}" << std::endl;

        std::cout << "o: { " << o.first << ", {Age: " << o.second.get_age()
                  << ", Name: " << o.second.get_name() << "}" << std::endl;

        std::cout << "p: ";
        if (p != hpx::nullopt)
            std::cout << *p << std::endl;
        else
            std::cout << "nullopt" << std::endl;
    }
};

class nested_object
{
private:
    int id;
    std::string name;
    person p;
    complicated_object o;

public:
    nested_object()
      : id(0)
    {
    }    // For deserialization
    nested_object(int id_val, std::string name_val, person p_val,
        complicated_object o_val)
      : id(id_val)
      , name(std::move(name_val))
      , p(std::move(p_val))
      , o(std::move(o_val))
    {
    }

    bool operator==(nested_object const& rhs) const
    {
        return id == rhs.id && name == rhs.name && p == rhs.p && o == rhs.o;
    }

    void print() const
    {
        std::cout << "ID: " << id << ", Name: " << name << std::endl;
        std::cout << "Person - Age: " << p.get_age()
                  << ", Name: " << p.get_name() << std::endl;
        o.print();
    }
};

int main()
{
    complicated_object input_o(
        Color::black, 42, "hello", {{10, "c1"}, {11, "c2"}},    // c: vector
        {"list1", "list2"},                                     // d: list
        {1, 2, 3},                                              // e: deque
        {{1, {10, "f1"}}, {2, {11, "f2"}}},                     // f: map
        {{1, {10, "g1"}}, {1, {11, "g2"}}},                     // g: multimap
        {"set1", "set2"},                                       // h: set
        {1, 1, 2, 3, 3, 3},                                     // i: multiset
        {{1, {10, "j1"}}},           // j: unordered_map
        {{1, 100}, {1, 101}},        // k: unordered_multimap
        {{{1, "m1"}, {2, "m2"}}},    // m: array
        // {1, 2},                       // m: array
        {"pair1", {1, "o1"}},         // o: pair
        123,                          // p: optional
        std::make_unique<int>(456)    // q: unique_ptr
    );

    nested_object input_data(2, "tom", {20, "tom"}, std::move(input_o));

    // Serialize
    std::vector<char> buffer;
    hpx::serialization::output_archive oarchive(buffer);
    oarchive << input_data;

    // Deserialize
    hpx::serialization::input_archive iarchive(buffer);
    nested_object output_data;
    iarchive >> output_data;

    // Print both
    std::cout << "Input Data:" << std::endl;
    input_data.print();
    std::cout << "\nOutput Data:" << std::endl;
    output_data.print();

    HPX_TEST(input_data == output_data);

    return hpx::util::report_errors();
}
