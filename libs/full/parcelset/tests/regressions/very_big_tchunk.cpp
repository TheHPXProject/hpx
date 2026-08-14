//  Copyright (c) 2025 Jiakun Yan
//  Copyright (c) 2025 Marco Diers
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/future.hpp>
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/runtime_distributed.hpp>

class Data
{
public:
    Data() = default;

    Data(std::size_t size)
    {
        for (auto i(0ul); i < size; ++i)
        {
            _data.emplace(std::make_tuple(i), std::vector<float>(3001));
        }
    }

    auto size() const
    {
        return _data.size();
    }

    template <typename Archive>
    friend auto serialize(Archive& archive, Data& object, unsigned int)
    {
        archive & object._data;
        return;
    }

private:
    std::map<std::tuple<std::size_t>, std::vector<float>> _data{};
};

class Component : public hpx::components::component_base<Component>
{
public:
    Component() = default;

    auto call(Data data) -> void
    {
        std::cout << "Data size: " << data.size() << '\n';
        return;
    }

    HPX_DEFINE_COMPONENT_ACTION(Component, call)
};

HPX_REGISTER_COMPONENT(hpx::components::component<Component>, Component);
HPX_REGISTER_ACTION(Component::call_action)

class ComponentClient
  : public hpx::components::client_base<ComponentClient, Component>
{
    using BaseType = hpx::components::client_base<ComponentClient, Component>;

public:
    template <typename... Arguments>
    ComponentClient(Arguments... arguments)
      : BaseType(std::move(arguments)...)
    {
    }

    template <typename... Arguments>
    auto call(Arguments... arguments)
    {
        return hpx::async<Component::call_action>(
            this->get_id(), std::move(arguments)...);
    }
};

int hpx_main()
{
    std::vector<ComponentClient> clients;
    auto localities(hpx::find_all_localities());
    std::transform(std::begin(localities), std::end(localities),
        std::back_inserter(clients),
        [](auto& loc) { return hpx::new_<ComponentClient>(loc); });
    Data data(1444);
    std::vector<decltype(clients.front().call(data))> calls;
    for (auto& client : clients)
    {
        calls.emplace_back(client.call(data));
    }
    hpx::wait_all(calls);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::init(argc, argv);
}
