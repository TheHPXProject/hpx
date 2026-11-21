//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// HPX's macros refer to HPX by its fully qualified name, so they can be used
// inside a namespace that has a nested namespace hpx of its own (as for
// instance thrust::system::hpx): an unqualified hpx:: would find that one.

#include <hpx/assert.hpp>
#include <hpx/future.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/include/serialization.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/testing.hpp>

#include <stdexcept>

namespace app {

    namespace hpx {

        // hides ::hpx inside namespace app
        struct unrelated
        {
        };
    }    // namespace hpx

    // used at namespace scope
    int increment(int i)
    {
        return i + 1;
    }
    HPX_DEFINE_PLAIN_ACTION(increment, increment_action);

    // used at class scope
    struct data
    {
        int value = 0;

        template <typename Archive>
        void save(Archive& ar, unsigned) const
        {
            ar << value;
        }

        template <typename Archive>
        void load(Archive& ar, unsigned)
        {
            ar >> value;
        }

        HPX_SERIALIZATION_SPLIT_MEMBER()
    };

    int get_value(data const& d)
    {
        return d.value;
    }
    HPX_DEFINE_PLAIN_ACTION(get_value, get_value_action);

    struct server : ::hpx::components::component_base<server>
    {
        int answer() const
        {
            return 42;
        }
        HPX_DEFINE_COMPONENT_ACTION(server, answer, answer_action)
    };

    // used at function scope
    void test_assertions(int const i)
    {
        HPX_ASSERT(i == 1);
        HPX_ASSERT_MSG(i == 1, "i is {}", i);
    }

    void test_exceptions()
    {
        bool caught = false;
        try
        {
            HPX_THROW_EXCEPTION(::hpx::error::bad_parameter, "test_exceptions",
                "expected error {}", 42);
        }
        catch (::hpx::exception const& e)
        {
            caught = e.get_error() == ::hpx::error::bad_parameter;
        }
        HPX_TEST(caught);

        ::hpx::error_code ec(::hpx::throwmode::lightweight);
        HPX_THROWS_IF(
            ec, ::hpx::error::bad_parameter, "test_exceptions", "expected");
        HPX_TEST(ec);

        ::hpx::future<int> f = HPX_MAKE_EXCEPTIONAL_FUTURE(
            int, ::hpx::error::bad_parameter, "test_exceptions", "expected");
        HPX_TEST(f.has_exception());
    }

    void test_logging()
    {
        LTM_(debug) << "macros in a namespace with a nested namespace hpx";
        LERR_(debug).format("{}", 42);
        LAPP_(debug) << "macros in a namespace with a nested namespace hpx";
    }

    void test_actions()
    {
        ::hpx::id_type const here = ::hpx::find_here();

        HPX_TEST_EQ(::hpx::async<increment_action>(here, 1).get(), 2);

        data d;
        d.value = 7;
        HPX_TEST_EQ(::hpx::async<get_value_action>(here, d).get(), 7);

        ::hpx::id_type const id = ::hpx::new_<server>(here).get();
        HPX_TEST_EQ(::hpx::async<server::answer_action>(id).get(), 42);
    }
}    // namespace app

HPX_REGISTER_ACTION_DECLARATION(app::increment_action, app_increment_action)
HPX_REGISTER_ACTION(app::increment_action, app_increment_action)
HPX_REGISTER_ACTION_DECLARATION(app::get_value_action, app_get_value_action)
HPX_REGISTER_ACTION(app::get_value_action, app_get_value_action)

using app_server_type = hpx::components::component<app::server>;
HPX_REGISTER_COMPONENT(app_server_type, app_server)
HPX_REGISTER_ACTION_DECLARATION(app::server::answer_action, app_answer_action)
HPX_REGISTER_ACTION(app::server::answer_action, app_answer_action)

int main()
{
    app::test_assertions(1);
    app::test_exceptions();
    app::test_logging();
    app::test_actions();

    return hpx::util::report_errors();
}
