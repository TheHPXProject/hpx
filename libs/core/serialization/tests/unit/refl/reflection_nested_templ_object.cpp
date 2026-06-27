//  Copyright (c) 2026 Ujjwal Shekhar
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/datastructures/optional.hpp>
#include <hpx/datastructures/serialization/optional.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <array>
#include <cstddef>
#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// POD struct for bitwise-safe array testing
struct Coordinate
{
    double x, y, z;

    bool operator==(Coordinate const& rhs) const
    {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }
};

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

    bool operator<(person const& rhs) const
    {
        if (age != rhs.age)
            return age < rhs.age;
        return name < rhs.name;
    }
};

class deeply_nested_object
{
private:
    // Array of PODs
    std::array<Coordinate, 4> fixed_points;
    // Template-of-template-of-template
    std::vector<std::map<int, std::vector<person>>> registry_history;
    // Mixed types with optionals
    hpx::optional<std::list<std::pair<std::string, person>>> metadata;

    struct internal_stats
    {
        double score;
        std::vector<std::string> tags;

        bool operator==(internal_stats const& rhs) const
        {
            return score == rhs.score && tags == rhs.tags;
        }
    };
    internal_stats stats;

public:
    deeply_nested_object() = default;

    deeply_nested_object(std::array<Coordinate, 4> pts,
        std::vector<std::map<int, std::vector<person>>> history,
        hpx::optional<std::list<std::pair<std::string, person>>> meta,
        double sc, std::vector<std::string> tgs)
      : fixed_points(pts)
      , registry_history(std::move(history))
      , metadata(std::move(meta))
      , stats{sc, std::move(tgs)}
    {
    }

    bool operator==(deeply_nested_object const& rhs) const
    {
        return fixed_points == rhs.fixed_points &&
            registry_history == rhs.registry_history &&
            metadata == rhs.metadata && stats == rhs.stats;
    }

    void print() const
    {
        std::cout << "--- Deeply Nested Object (POD Array) ---" << std::endl;
        std::cout << "Fixed Points: " << std::endl;
        for (std::size_t i = 0; i < fixed_points.size(); ++i)
        {
            std::cout << "Point " << i << ": (" << fixed_points[i].x << ", "
                      << fixed_points[i].y << ", " << fixed_points[i].z << ")"
                      << std::endl;
        }
        std::cout << "History Groups: " << registry_history.size() << std::endl;
        for (auto const& group : registry_history)
        {
            for (auto const& [id, persons] : group)
            {
                std::cout << "ID: " << id << " -> Persons: ";
                for (auto const& p : persons)
                {
                    std::cout << "{" << p.get_age() << ", " << p.get_name()
                              << "} ";
                }
                std::cout << std::endl;
            }
        }
        if (metadata != hpx::nullopt)
        {
            std::cout << "Metadata entries: " << metadata->size() << std::endl;
            for (auto const& [key, p] : *metadata)
            {
                std::cout << "Key: " << key << " -> Person: {" << p.get_age()
                          << ", " << p.get_name() << "}" << std::endl;
            }
        }
        std::cout << "Stats Score: " << stats.score << std::endl;
        std::cout << "Stats Tags: ";
        for (auto const& tag : stats.tags)
        {
            std::cout << tag << " ";
        }
        std::cout << std::endl;
    }
};

int main()
{
    // Setup
    std::array<Coordinate, 4> pts = {
        {{1.1, 2.2, 3.3}, {4.4, 5.5, 6.6}, {0.0, 0.0, 0.0}, {-1.0, 2.0, 5.0}}};

    std::vector<person> group_a = {{20, "Alice"}, {25, "Bob"}};
    std::map<int, std::vector<person>> map_1;
    map_1[101] = group_a;

    std::vector<std::map<int, std::vector<person>>> history;
    history.push_back(map_1);

    std::list<std::pair<std::string, person>> meta_list = {
        {"Admin", {50, "Root"}}};

    deeply_nested_object input_data(pts, std::move(history), meta_list, 99.5,
        {"reflection", "test", "pod_array"});

    // Serialize
    std::vector<char> buffer;
    hpx::serialization::output_archive oarchive(buffer);
    oarchive << input_data;

    // Deserialize
    hpx::serialization::input_archive iarchive(buffer);
    deeply_nested_object output_data;
    iarchive >> output_data;

    // Verify
    input_data.print();
    output_data.print();

    HPX_TEST(input_data == output_data);

    return hpx::util::report_errors();
}
