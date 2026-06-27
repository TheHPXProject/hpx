//  Copyright (c) 2026 Abhishek Bansal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Unit tests for the static plugin registry infrastructure that supports
// static linking of HPX plugins (binary filters, message handlers, etc.)
// and plugins implemented directly inside the application executable.

// HPX_PLUGIN_NAME must be defined before <hpx/config.hpp> so that
// config.hpp can derive HPX_PLUGIN_PLUGIN_PREFIX from it. This stands in
// for the target_compile_definition CMake adds for plugin targets; here
// the "plugin" lives in the test executable itself.
#define HPX_PLUGIN_NAME app_local_plugin

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/plugin.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Minimal plugin_registry_base implementations for testing.
// get_plugin_info() emits ini lines in the same format real plugin registries
// use (see plugin_registry::get_plugin_info in plugin_factories).

struct fake_plugin_registry : hpx::plugins::plugin_registry_base
{
    bool get_plugin_info(std::vector<std::string>& fillini) override
    {
        fillini.emplace_back("[hpx.plugins.fake_test_plugin_instance]");
        fillini.emplace_back("name = fake_test_plugin_instance");
        fillini.emplace_back("enabled = 1");
        return true;
    }
};

// Two additional registries that live in the same exported-plugins map.
// Used by test_load_plugin_factory_static_multi_section to verify that
// the static = 1 injection logic handles two consecutive [hpx.plugins.*]
// sections correctly (the fragile part of the section-rewriting loop).
struct fake_plugin_registry_alpha : hpx::plugins::plugin_registry_base
{
    bool get_plugin_info(std::vector<std::string>& fillini) override
    {
        fillini.emplace_back("[hpx.plugins.fake_alpha]");
        fillini.emplace_back("name = fake_alpha");
        fillini.emplace_back("enabled = 1");
        return true;
    }
};

struct fake_plugin_registry_beta : hpx::plugins::plugin_registry_base
{
    bool get_plugin_info(std::vector<std::string>& fillini) override
    {
        fillini.emplace_back("[hpx.plugins.fake_beta]");
        fillini.emplace_back("name = fake_beta");
        fillini.emplace_back("enabled = 1");
        return true;
    }
};

///////////////////////////////////////////////////////////////////////////////
// Set up the plugin export infrastructure manually, mirroring what the
// HPX_PLUGIN_EXPORT_LIST + HPX_PLUGIN_EXPORT macros expand to:
//
// 1. A function returning a pointer to a static map (exported plugins list)
// 2. A concrete_factory<plugin_registry_base, fake_plugin_registry> entry
//    registered in that map under the lowercase plugin name

static std::map<std::string, hpx::any_nonser>& get_fake_plugin_map()
{
    static std::map<std::string, hpx::any_nonser> r;
    return r;
}

static std::map<std::string, hpx::any_nonser>* HPX_PLUGIN_API
fake_get_plugins_list()
{
    return &get_fake_plugin_map();
}

// Register the concrete factory into the map at static-init time.
static struct fake_plugin_exporter
{
    fake_plugin_exporter()
    {
        static hpx::util::plugin::concrete_factory<
            hpx::plugins::plugin_registry_base, fake_plugin_registry>
            cf;
        hpx::util::plugin::abstract_factory<hpx::plugins::plugin_registry_base>*
            w = &cf;
        get_fake_plugin_map().insert(
            {"fake_test_plugin_instance", hpx::any_nonser(w)});
    }
} fake_exporter_instance;

///////////////////////////////////////////////////////////////////////////////
// A second exported-plugins map with TWO registries. This exercises the
// multi-section path through load_plugin_factory_static's injection loop.

static std::map<std::string, hpx::any_nonser>& get_multi_plugin_map()
{
    static std::map<std::string, hpx::any_nonser> r;
    return r;
}

static std::map<std::string, hpx::any_nonser>* HPX_PLUGIN_API
multi_get_plugins_list()
{
    return &get_multi_plugin_map();
}

static struct multi_plugin_exporter
{
    multi_plugin_exporter()
    {
        static hpx::util::plugin::concrete_factory<
            hpx::plugins::plugin_registry_base, fake_plugin_registry_alpha>
            cf_alpha;
        static hpx::util::plugin::concrete_factory<
            hpx::plugins::plugin_registry_base, fake_plugin_registry_beta>
            cf_beta;

        hpx::util::plugin::abstract_factory<hpx::plugins::plugin_registry_base>*
            wa = &cf_alpha;
        hpx::util::plugin::abstract_factory<hpx::plugins::plugin_registry_base>*
            wb = &cf_beta;

        get_multi_plugin_map().insert({"fake_alpha", hpx::any_nonser(wa)});
        get_multi_plugin_map().insert({"fake_beta", hpx::any_nonser(wb)});
    }
} multi_exporter_instance;

struct app_local_plugin_registry : hpx::plugins::plugin_registry_base
{
    bool get_plugin_info(std::vector<std::string>& fillini) override
    {
        fillini.emplace_back("[hpx.plugins.app_local_plugin_instance]");
        fillini.emplace_back("name = app_local_plugin_instance");
        fillini.emplace_back("enabled = 1");
        return true;
    }
};

HPX_REGISTER_PLUGIN_BASE_REGISTRY(
    app_local_plugin_registry, app_local_plugin_instance)
HPX_REGISTER_PLUGIN_REGISTRY_MODULE()

///////////////////////////////////////////////////////////////////////////////
// Test: init_registry_plugin_module stores a plugin module entry and
//       get_static_plugin_module_data returns it.
void test_init_registry_plugin_module()
{
    auto const size_before =
        hpx::components::get_static_plugin_module_data().size();

    hpx::components::static_factory_load_data_type const data{
        "fake_test_plugin_module", &fake_get_plugins_list};
    hpx::components::init_registry_plugin_module(data);

    auto const& modules = hpx::components::get_static_plugin_module_data();
    HPX_TEST(modules.size() > size_before);

    bool found = false;
    for (auto const& entry : modules)
    {
        if (std::string(entry.name) == "fake_test_plugin_module")
        {
            HPX_TEST(entry.get_factory == &fake_get_plugins_list);
            found = true;
            break;
        }
    }
    HPX_TEST(found);
}

///////////////////////////////////////////////////////////////////////////////
// Test: init_registry_plugin_factory stores a factory entry and
//       get_static_plugin_factory can retrieve it by instance name.
void test_init_registry_plugin_factory()
{
    hpx::components::static_factory_load_data_type const data{
        "fake_test_plugin_instance", &fake_get_plugins_list};
    hpx::components::init_registry_plugin_factory(data);

    hpx::util::plugin::get_plugins_list_type f = nullptr;
    bool const ok = hpx::components::get_static_plugin_factory(
        "fake_test_plugin_instance", f);

    HPX_TEST(ok);
    HPX_TEST(f == &fake_get_plugins_list);
}

///////////////////////////////////////////////////////////////////////////////
// Test: get_static_plugin_factory returns false for an unknown name.
void test_get_static_plugin_factory_not_found()
{
    hpx::util::plugin::get_plugins_list_type f = nullptr;
    bool const ok = hpx::components::get_static_plugin_factory(
        "nonexistent_plugin_xyz_12345", f);

    HPX_TEST(!ok);
    HPX_TEST(f == nullptr);
}

///////////////////////////////////////////////////////////////////////////////
// Test: load_plugin_factory_static creates an ini section with
//       [hpx.plugins.<instance>] containing static = 1, and returns a
//       non-empty vector of registry objects.
void test_load_plugin_factory_static()
{
    hpx::util::section ini;

    hpx::error_code ec(hpx::throwmode::lightweight);
    auto registries = hpx::util::load_plugin_factory_static(
        ini, "fake_test_plugin_instance", &fake_get_plugins_list, ec);

    HPX_TEST(!ec);
    HPX_TEST(!registries.empty());

    HPX_TEST(ini.has_section("hpx.plugins.fake_test_plugin_instance"));
    if (ini.has_section("hpx.plugins.fake_test_plugin_instance"))
    {
        auto* sect = ini.get_section("hpx.plugins.fake_test_plugin_instance");
        HPX_TEST(sect != nullptr);

        if (sect != nullptr)
        {
            // Key assertion: static = 1 was injected by
            // load_plugin_factory_static
            std::string const static_val = sect->get_entry("static", "0");
            HPX_TEST_EQ(static_val, std::string("1"));

            std::string const name_val = sect->get_entry("name", "");
            HPX_TEST_EQ(name_val, std::string("fake_test_plugin_instance"));

            std::string const enabled_val = sect->get_entry("enabled", "0");
            HPX_TEST_EQ(enabled_val, std::string("1"));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// An empty plugins list function - exports no factories.
static std::map<std::string, hpx::any_nonser>* HPX_PLUGIN_API
empty_get_plugins_list()
{
    static std::map<std::string, hpx::any_nonser> m;
    return &m;
}

// Test: load_plugin_factory_static with an empty factory (no names exported)
//       returns an empty vector and does not create any section.
void test_load_plugin_factory_static_empty()
{
    hpx::util::section ini;
    hpx::error_code ec(hpx::throwmode::lightweight);
    auto registries = hpx::util::load_plugin_factory_static(
        ini, "empty_plugin", &empty_get_plugins_list, ec);

    HPX_TEST(!ec);
    HPX_TEST(registries.empty());
    HPX_TEST(!ini.has_section("hpx.plugins.empty_plugin"));
}

///////////////////////////////////////////////////////////////////////////////
// Test: load_plugin_factory_static with two registries in one module.
// The injection loop must insert static = 1 into BOTH [hpx.plugins.fake_alpha]
// and [hpx.plugins.fake_beta]. This exercises the state machine that tracks
// section headers - a bug here (e.g. only marking the first or last section)
// would silently break multi-plugin modules in static builds.
void test_load_plugin_factory_static_multi_section()
{
    hpx::util::section ini;

    hpx::error_code ec(hpx::throwmode::lightweight);
    auto registries = hpx::util::load_plugin_factory_static(
        ini, "multi_test", &multi_get_plugins_list, ec);

    HPX_TEST(!ec);
    HPX_TEST_EQ(registries.size(), std::size_t(2));

    // Both plugin sections must exist and both must have static = 1.
    HPX_TEST(ini.has_section("hpx.plugins.fake_alpha"));
    if (ini.has_section("hpx.plugins.fake_alpha"))
    {
        auto* sect = ini.get_section("hpx.plugins.fake_alpha");
        HPX_TEST(sect != nullptr);
        if (sect != nullptr)
        {
            HPX_TEST_EQ(sect->get_entry("static", "0"), std::string("1"));
            HPX_TEST_EQ(sect->get_entry("name", ""), std::string("fake_alpha"));
        }
    }

    HPX_TEST(ini.has_section("hpx.plugins.fake_beta"));
    if (ini.has_section("hpx.plugins.fake_beta"))
    {
        auto* sect = ini.get_section("hpx.plugins.fake_beta");
        HPX_TEST(sect != nullptr);
        if (sect != nullptr)
        {
            HPX_TEST_EQ(sect->get_entry("static", "0"), std::string("1"));
            HPX_TEST_EQ(sect->get_entry("name", ""), std::string("fake_beta"));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// End-to-end test for the macro-based in-app plugin registration above.
// Finds the entry pushed by HPX_REGISTER_PLUGIN_REGISTRY_MODULE's static
// ctor, then runs it through load_plugin_factory_static and asserts the
// generated ini section matches what app_local_plugin_registry produced.
void test_app_local_plugin_via_macros()
{
    hpx::util::plugin::get_plugins_list_type get_factory = nullptr;
    for (auto const& entry : hpx::components::get_static_plugin_module_data())
    {
        if (std::string(entry.name) == "app_local_plugin")
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
    auto registries = hpx::util::load_plugin_factory_static(
        ini, "app_local_plugin", get_factory, ec);

    HPX_TEST(!ec);
    HPX_TEST(!registries.empty());

    HPX_TEST(ini.has_section("hpx.plugins.app_local_plugin_instance"));
    if (ini.has_section("hpx.plugins.app_local_plugin_instance"))
    {
        auto* sect = ini.get_section("hpx.plugins.app_local_plugin_instance");
        HPX_TEST(sect != nullptr);
        if (sect != nullptr)
        {
            HPX_TEST_EQ(sect->get_entry("static", "0"), std::string("1"));
            HPX_TEST_EQ(sect->get_entry("name", ""),
                std::string("app_local_plugin_instance"));
            HPX_TEST_EQ(sect->get_entry("enabled", "0"), std::string("1"));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    test_init_registry_plugin_module();
    test_init_registry_plugin_factory();
    test_get_static_plugin_factory_not_found();
    test_load_plugin_factory_static();
    test_load_plugin_factory_static_empty();
    test_load_plugin_factory_static_multi_section();
    test_app_local_plugin_via_macros();

    return hpx::util::report_errors();
}
