//  Copyright (c) 2026 Abhishek Bansal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Unit tests for the static component registry infrastructure that supports
// static linking of HPX components and components implemented directly inside
// the application executable.

// HPX_COMPONENT_NAME must be defined before <hpx/config.hpp> so that
// config.hpp can derive HPX_PLUGIN_COMPONENT_PREFIX from it. This stands in
// for the target_compile_definition CMake adds for component targets; here
// the "component" lives in the test executable itself.
#define HPX_COMPONENT_NAME app_local_component

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/plugin.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/runtime_configuration/component_registry_base.hpp>

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Minimal component_registry_base implementations for testing.
// get_component_info() emits ini lines in the same format real component
// registries use. The is_static parameter is how load_component_factory_static
// tells registries to stamp `static = 1` into their sections.

struct fake_component_registry : hpx::components::component_registry_base
{
    bool get_component_info(std::vector<std::string>& fillini,
        std::string const& /*filepath*/, bool is_static = false) override
    {
        fillini.emplace_back("[hpx.components.fake_test_component_instance]");
        fillini.emplace_back("name = fake_test_component_instance");
        fillini.emplace_back("enabled = 1");
        if (is_static)
            fillini.emplace_back("static = 1");
        return true;
    }

    void register_component_type() override {}
};

///////////////////////////////////////////////////////////////////////////////
// Set up the plugin export infrastructure manually, mirroring what
// HPX_PLUGIN_EXPORT_LIST + HPX_PLUGIN_EXPORT expand to:
//
// 1. A function returning a pointer to a static map (exported registries list)
// 2. A concrete_factory<component_registry_base, fake_component_registry>
//    entry registered in that map under the lowercase component name

static std::map<std::string, hpx::any_nonser>& get_fake_component_map()
{
    static std::map<std::string, hpx::any_nonser> r;
    return r;
}

static std::map<std::string, hpx::any_nonser>* HPX_PLUGIN_API
fake_get_components_list()
{
    return &get_fake_component_map();
}

static struct fake_component_exporter
{
    fake_component_exporter()
    {
        static hpx::util::plugin::concrete_factory<
            hpx::components::component_registry_base, fake_component_registry>
            cf;
        hpx::util::plugin::abstract_factory<
            hpx::components::component_registry_base>* w = &cf;
        get_fake_component_map().insert(
            {"fake_test_component_instance", hpx::any_nonser(w)});
    }
} fake_exporter_instance;

///////////////////////////////////////////////////////////////////////////////
// An app-local component registry registered via the public macros. The
// static ctor emitted by HPX_INIT_REGISTRY_MODULE_STATIC (inside
// HPX_REGISTER_REGISTRY_MODULE, which HPX_REGISTER_COMPONENT_MODULE expands
// to) pushes an entry into get_static_module_data() at program start.
// test_app_local_component_via_macros reads that entry back.

struct app_local_component_registry : hpx::components::component_registry_base
{
    bool get_component_info(std::vector<std::string>& fillini,
        std::string const& /*filepath*/, bool is_static = false) override
    {
        fillini.emplace_back("[hpx.components.app_local_component_instance]");
        fillini.emplace_back("name = app_local_component_instance");
        fillini.emplace_back("enabled = 1");
        if (is_static)
            fillini.emplace_back("static = 1");
        return true;
    }

    void register_component_type() override {}
};

HPX_REGISTER_COMPONENT_REGISTRY(
    app_local_component_registry, app_local_component_instance)
HPX_REGISTER_COMPONENT_MODULE()

///////////////////////////////////////////////////////////////////////////////
// Test: init_registry_module stores a component module entry and
//       get_static_module_data returns it.
void test_init_registry_module()
{
    auto const size_before = hpx::components::get_static_module_data().size();

    hpx::components::static_factory_load_data_type const data{
        "fake_test_component_module", &fake_get_components_list};
    hpx::components::init_registry_module(data);

    auto const& modules = hpx::components::get_static_module_data();
    HPX_TEST(modules.size() > size_before);

    bool found = false;
    for (auto const& entry : modules)
    {
        if (std::string(entry.name) == "fake_test_component_module")
        {
            HPX_TEST(entry.get_factory == &fake_get_components_list);
            found = true;
            break;
        }
    }
    HPX_TEST(found);
}

///////////////////////////////////////////////////////////////////////////////
// Test: init_registry_factory stores a factory entry and get_static_factory
//       can retrieve it by instance name.
void test_init_registry_factory()
{
    hpx::components::static_factory_load_data_type const data{
        "fake_test_component_instance", &fake_get_components_list};
    hpx::components::init_registry_factory(data);

    hpx::util::plugin::get_plugins_list_type f = nullptr;
    bool const ok =
        hpx::components::get_static_factory("fake_test_component_instance", f);

    HPX_TEST(ok);
    HPX_TEST(f == &fake_get_components_list);
}

///////////////////////////////////////////////////////////////////////////////
// Test: get_static_factory returns false for an unknown name.
void test_get_static_factory_not_found()
{
    hpx::util::plugin::get_plugins_list_type f = nullptr;
    bool const ok = hpx::components::get_static_factory(
        "nonexistent_component_xyz_12345", f);

    HPX_TEST(!ok);
    HPX_TEST(f == nullptr);
}

///////////////////////////////////////////////////////////////////////////////
// Test: load_component_factory_static creates an ini section with
//       [hpx.components.<instance>] containing static = 1, and returns a
//       non-empty vector of registry objects. static = 1 comes from the
//       registry's get_component_info(is_static=true) call, not a rewrite
//       pass (that's the plugin path).
void test_load_component_factory_static()
{
    hpx::util::section ini;

    hpx::error_code ec(hpx::throwmode::lightweight);
    auto registries = hpx::util::load_component_factory_static(
        ini, "fake_test_component_module", &fake_get_components_list, ec);

    HPX_TEST(!ec);
    HPX_TEST(!registries.empty());

    HPX_TEST(ini.has_section("hpx.components.fake_test_component_instance"));
    if (ini.has_section("hpx.components.fake_test_component_instance"))
    {
        auto* sect =
            ini.get_section("hpx.components.fake_test_component_instance");
        HPX_TEST(sect != nullptr);

        if (sect != nullptr)
        {
            HPX_TEST_EQ(sect->get_entry("static", "0"), std::string("1"));
            HPX_TEST_EQ(sect->get_entry("name", ""),
                std::string("fake_test_component_instance"));
            HPX_TEST_EQ(sect->get_entry("enabled", "0"), std::string("1"));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// An empty components list function - exports no factories. Exercises the
// `no_factory = 1` default-section branch in load_component_factory_static
// (init_ini_data.cpp ~line 246-256): a module may export only startup/
// shutdown functions, and the loader synthesizes a placeholder section with
// static = 1 so the runtime still knows about the module.
static std::map<std::string, hpx::any_nonser>* HPX_PLUGIN_API
empty_get_components_list()
{
    static std::map<std::string, hpx::any_nonser> m;
    return &m;
}

void test_load_component_factory_static_empty()
{
    hpx::util::section ini;
    hpx::error_code ec(hpx::throwmode::lightweight);
    auto registries = hpx::util::load_component_factory_static(
        ini, "empty_component", &empty_get_components_list, ec);

    HPX_TEST(!ec);
    HPX_TEST(registries.empty());

    HPX_TEST(ini.has_section("hpx.components.empty_component"));
    if (ini.has_section("hpx.components.empty_component"))
    {
        auto* sect = ini.get_section("hpx.components.empty_component");
        HPX_TEST(sect != nullptr);
        if (sect != nullptr)
        {
            HPX_TEST_EQ(sect->get_entry("static", "0"), std::string("1"));
            HPX_TEST_EQ(sect->get_entry("no_factory", "0"), std::string("1"));
            HPX_TEST_EQ(sect->get_entry("enabled", "0"), std::string("1"));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// End-to-end test for the macro-based in-app component registration above.
// Finds the entry pushed by HPX_REGISTER_COMPONENT_MODULE's static ctor,
// then runs it through load_component_factory_static and asserts the
// generated ini section matches what app_local_component_registry produced.
// This is the scenario the app-exe-ungating change unlocks.
void test_app_local_component_via_macros()
{
    hpx::util::plugin::get_plugins_list_type get_factory = nullptr;
    for (auto const& entry : hpx::components::get_static_module_data())
    {
        if (std::string(entry.name) == "app_local_component")
        {
            get_factory = entry.get_factory;
            break;
        }
    }
    HPX_TEST(get_factory != nullptr);
    if (get_factory == nullptr)
        return;

    hpx::util::section ini;
    hpx::error_code ec(hpx::throwmode::lightweight);
    auto registries = hpx::util::load_component_factory_static(
        ini, "app_local_component", get_factory, ec);

    HPX_TEST(!ec);
    HPX_TEST(!registries.empty());

    HPX_TEST(ini.has_section("hpx.components.app_local_component_instance"));
    if (ini.has_section("hpx.components.app_local_component_instance"))
    {
        auto* sect =
            ini.get_section("hpx.components.app_local_component_instance");
        HPX_TEST(sect != nullptr);
        if (sect != nullptr)
        {
            HPX_TEST_EQ(sect->get_entry("static", "0"), std::string("1"));
            HPX_TEST_EQ(sect->get_entry("name", ""),
                std::string("app_local_component_instance"));
            HPX_TEST_EQ(sect->get_entry("enabled", "0"), std::string("1"));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    test_init_registry_module();
    test_init_registry_factory();
    test_get_static_factory_not_found();
    test_load_component_factory_static();
    test_load_component_factory_static_empty();
    test_app_local_component_via_macros();

    return hpx::util::report_errors();
}
