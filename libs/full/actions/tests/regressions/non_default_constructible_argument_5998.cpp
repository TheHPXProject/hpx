//  Copyright (c) 2022 Julien Esseiva
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class EventContext
{
public:
    EventContext(std::size_t id, std::vector<std::string> requested)
      : _id{id}
      , _requested{std::move(requested)}
    {
    }

    template <typename Archive>
    friend void save_construct_data(
        Archive&, EventContext const*, unsigned int const);

    [[nodiscard]] std::size_t id() const
    {
        return _id;
    };
    [[nodiscard]] std::size_t& id()
    {
        return _id;
    };
    [[nodiscard]] std::vector<std::string> const& requested() const
    {
        return _requested;
    };

private:
    friend class hpx::serialization::access;
    std::size_t _id;
    std::vector<std::string> _requested;

    template <typename Archive>
    void serialize(Archive&, unsigned int const)
    {
    }
};

template <class Archive>
inline void save_construct_data(
    Archive& ar, EventContext const* ec, unsigned int const)
{
    ar << ec->_id << ec->_requested;
}

template <class Archive>
inline void load_construct_data(
    Archive& ar, EventContext* ec, unsigned int const)
{
    std::vector<std::string> requested;
    std::size_t eid;
    ar >> eid >> requested;

    ::new (ec) EventContext(eid, std::move(requested));
}

void print_params(
    EventContext const& eventContext, std::vector<std::string> const& data)
{
    using namespace std::string_literals;
    HPX_TEST(eventContext.id() == std::size_t(23));
    HPX_TEST(eventContext.requested() == std::vector({"a"s, "b"s}));
    HPX_TEST(data == std::vector({"foo"s, "bar"s}));
}
HPX_PLAIN_ACTION(print_params, print_params_action)

auto call_remote()
{
    using namespace std::string_literals;
    auto remote{hpx::find_remote_localities()};
    std::vector<hpx::future<void>> futures;
    for (auto const& id : remote)
    {
        EventContext ec{23, {"a"s, "b"s}};
        std::vector<std::string> data{"foo"s, "bar"s};
        futures.emplace_back(hpx::async<print_params_action>(
            id, std::move(ec), std::move(data)));
    }
    return hpx::when_all(futures);
}

int hpx_main()
{
    for (auto& future : call_remote().get())
    {
        future.get();
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
#endif
