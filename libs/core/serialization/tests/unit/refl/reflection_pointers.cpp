//  Copyright (c) 2026 Ujjwal Shekhar
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

// A simple class
class ReflStruct
{
private:
    int i;
    std::string s;

public:
    ReflStruct() = default;
    ReflStruct(int i, std::string s)
      : i(i)
      , s(std::move(s))
    {
    }

    bool operator==(ReflStruct const& rhs) const
    {
        return i == rhs.i && s == rhs.s;
    }
};

// A host struct containing smart pointers
// This struct has NO serialize() function.
struct PointerHost
{
    std::unique_ptr<ReflStruct> u_ptr_full;
    std::unique_ptr<ReflStruct> u_ptr_null;
    std::shared_ptr<ReflStruct> s_ptr_full;
    std::shared_ptr<ReflStruct> s_ptr_null;

    // No easy operator== for unique_ptr, we'll test manually
};

int main()
{
    std::vector<char> buffer;
    hpx::serialization::output_archive oarchive(buffer);

    PointerHost input_data;
    input_data.u_ptr_full = std::make_unique<ReflStruct>(1, "one");
    input_data.u_ptr_null = nullptr;
    input_data.s_ptr_full = std::make_shared<ReflStruct>(2, "two");
    input_data.s_ptr_null = nullptr;

    oarchive << input_data;

    hpx::serialization::input_archive iarchive(buffer);
    PointerHost output_data;
    iarchive >> output_data;

    // Verify unique_ptrs
    HPX_TEST(output_data.u_ptr_null == nullptr);
    HPX_TEST(output_data.u_ptr_full != nullptr);
    HPX_TEST(*input_data.u_ptr_full == *output_data.u_ptr_full);

    // Verify shared_ptrs
    HPX_TEST(output_data.s_ptr_null == nullptr);
    HPX_TEST(output_data.s_ptr_full != nullptr);
    HPX_TEST(*input_data.s_ptr_full == *output_data.s_ptr_full);

    return hpx::util::report_errors();
}
