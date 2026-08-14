# Copyright (c) 2026 Pratyksh Gupta
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import copy, rmdir, load
from conan.tools.scm import Version
from conan.errors import ConanInvalidConfiguration
import os
import re

required_conan_version = ">=1.53.0"


class HPXConan(ConanFile):
    name = "hpx"
    license = "BSL-1.0"
    author = "The HPX Project"
    url = "https://github.com/TheHPXProject/hpx"
    homepage = "https://hpx.dev"
    description = "The C++ Standards Library for Parallelism and Concurrency"
    topics = ("hpx", "parallelism", "concurrency", "distributed-computing", "hpc", "runtime-system")

    settings = "os", "compiler", "build_type", "arch"

    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_networking": [True, False],
        "with_distributed_runtime": [True, False],
        "with_examples": [True, False],
        "with_tests": [True, False],
        "with_tools": [True, False],
        "with_cuda": [True, False],
        "with_hip": [True, False],
        "with_sycl": [True, False],
        "with_mpi": [True, False],
        "with_tcp": [True, False],
        "with_lci": [True, False],
        "with_apex": [True, False],
        "with_papi": [True, False],
        "with_valgrind": [True, False],
        "with_compression_zlib": [True, False],
        "with_compression_bzip2": [True, False],
        "with_compression_snappy": [True, False],
        "with_generic_context_coroutines": [True, False],
        "with_logging": [True, False],
        "with_stacktraces": [True, False],
        "malloc": ["system", "tcmalloc", "jemalloc", "mimalloc", "tbbmalloc"],
        "max_cpu_count": ["ANY"],
        "cxx_standard": ["20", "23", "26"],
    }

    default_options = {
        "shared": False,
        "fPIC": True,
        "with_networking": True,
        "with_distributed_runtime": True,
        "with_examples": False,
        "with_tests": False,
        "with_tools": False,
        "with_cuda": False,
        "with_hip": False,
        "with_sycl": False,
        "with_mpi": False,
        "with_tcp": True,
        "with_lci": False,
        "with_apex": False,
        "with_papi": False,
        "with_valgrind": False,
        "with_compression_zlib": True,
        "with_compression_bzip2": False,
        "with_compression_snappy": False,
        "with_generic_context_coroutines": False,
        "with_logging": True,
        "with_stacktraces": True,
        "malloc": "tcmalloc" if os.name != "nt" else "system",
        "max_cpu_count": "",
        "cxx_standard": "20",
    }

    exports_sources = "CMakeLists.txt", "cmake/*", "libs/*", "components/*", \
                      "examples/*", "tests/*", "tools/*", "docs/*", "wrap/*", \
                      "init/*", "LICENSE_1_0.txt", "README.rst", "*.cmake", \
                      "CMakePresets.json", "CTestConfig.cmake", "CITATION.cff", \
                      "hpx.spdx"

    def set_version(self):
        """Dynamically determine version from CMakeLists.txt"""
        cmake_file = os.path.join(self.recipe_folder, "CMakeLists.txt")
        content = load(self, cmake_file)
        major = re.search(r"set\(HPX_VERSION_MAJOR\s+(\d+)\)", content)
        minor = re.search(r"set\(HPX_VERSION_MINOR\s+(\d+)\)", content)
        patch = re.search(r"set\(HPX_VERSION_SUBMINOR\s+(\d+)\)", content)

        if major and minor and patch:
            self.version = f"{major.group(1)}.{minor.group(1)}.{patch.group(1)}"
        else:
            # Fallback to default if parsing fails
            self.version = "2.0.0"
            self.output.warning("Could not parse version from CMakeLists.txt, using default 2.0.0")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC
            self.options.malloc = "system"

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

        # Validate options
        if self.options.with_cuda and self.options.with_hip:
            raise ConanInvalidConfiguration("HPX cannot be built with both CUDA and HIP support")

        # Distributed runtime requires networking
        if self.options.with_distributed_runtime and not self.options.with_networking:
            raise ConanInvalidConfiguration("Distributed runtime requires networking to be enabled")

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            if Version(self.settings.compiler.cppstd) < "20":
                raise ConanInvalidConfiguration("HPX requires at least C++20")

        if self.settings.os == "Windows" and self.options.shared:
             # HPX on Windows has some issues with shared builds in some configurations
             self.output.warning("Shared builds on Windows might be unstable")

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        # Core dependencies
        self.requires("boost/1.86.0")
        self.requires("hwloc/2.10.0")
        self.requires("asio/1.30.2")

        # Optional dependencies
        if self.options.malloc == "tcmalloc":
            self.requires("gperftools/2.15")
        elif self.options.malloc == "jemalloc":
            self.requires("jemalloc/5.3.0")
        elif self.options.malloc == "mimalloc":
            self.requires("mimalloc/2.1.7")
        elif self.options.malloc == "tbbmalloc":
            self.requires("onetbb/2021.12.0")

        if self.options.with_compression_zlib:
            self.requires("zlib/[>=1.2.11]")
        if self.options.with_compression_bzip2:
            self.requires("bzip2/1.0.8")
        if self.options.with_compression_snappy:
            self.requires("snappy/1.1.9")

        if self.options.with_mpi:
            # Note: MPI is typically a system dependency, users may need to provide it
            self.output.info("MPI support enabled. Please ensure MPI is available on your system.")

        if self.options.with_apex:
            self.output.info("APEX support enabled. Please ensure APEX is available on your system.")

        if self.options.with_papi:
            self.output.info("PAPI support enabled. Please ensure PAPI is available on your system.")

        if self.options.with_valgrind:
            self.requires("valgrind/3.23.0")

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.18]")

    def generate(self):
        tc = CMakeToolchain(self)

        # Build type options
        tc.variables["HPX_WITH_STATIC_LINKING"] = not self.options.shared
        tc.variables["BUILD_SHARED_LIBS"] = self.options.shared

        # Feature options
        tc.variables["HPX_WITH_NETWORKING"] = self.options.with_networking
        tc.variables["HPX_WITH_DISTRIBUTED_RUNTIME"] = self.options.with_distributed_runtime
        tc.variables["HPX_WITH_EXAMPLES"] = self.options.with_examples
        tc.variables["HPX_WITH_TESTS"] = self.options.with_tests
        tc.variables["HPX_WITH_TOOLS"] = self.options.with_tools

        # Accelerator support
        tc.variables["HPX_WITH_CUDA"] = self.options.with_cuda
        tc.variables["HPX_WITH_HIP"] = self.options.with_hip
        tc.variables["HPX_WITH_SYCL"] = self.options.with_sycl

        # Parcelport options
        tc.variables["HPX_WITH_PARCELPORT_MPI"] = self.options.with_mpi
        tc.variables["HPX_WITH_PARCELPORT_TCP"] = self.options.with_tcp
        tc.variables["HPX_WITH_PARCELPORT_LCI"] = self.options.with_lci

        # Performance and debugging options
        tc.variables["HPX_WITH_MALLOC"] = str(self.options.malloc)
        tc.variables["HPX_WITH_APEX"] = self.options.with_apex
        tc.variables["HPX_WITH_PAPI"] = self.options.with_papi
        tc.variables["HPX_WITH_VALGRIND"] = self.options.with_valgrind
        tc.variables["HPX_WITH_COMPRESSION_ZLIB"] = self.options.with_compression_zlib
        tc.variables["HPX_WITH_COMPRESSION_BZIP2"] = self.options.with_compression_bzip2
        tc.variables["HPX_WITH_COMPRESSION_SNAPPY"] = self.options.with_compression_snappy
        tc.variables["HPX_WITH_GENERIC_CONTEXT_COROUTINES"] = self.options.with_generic_context_coroutines
        tc.variables["HPX_WITH_LOGGING"] = self.options.with_logging
        tc.variables["HPX_WITH_STACKTRACES"] = self.options.with_stacktraces

        # C++ standard
        tc.variables["HPX_WITH_CXX_STANDARD"] = str(self.options.cxx_standard)

        # Max CPU count
        if self.options.max_cpu_count:
            tc.variables["HPX_WITH_MAX_CPU_COUNT"] = str(self.options.max_cpu_count)

        # Disable documentation by default for package builds
        tc.variables["HPX_WITH_DOCUMENTATION"] = False

        # Use system-provided dependencies from Conan
        tc.variables["HPX_WITH_FETCH_BOOST"] = False
        tc.variables["HPX_WITH_FETCH_HWLOC"] = False
        tc.variables["HPX_WITH_FETCH_ASIO"] = False

        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "LICENSE_1_0.txt", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()

        # Remove unnecessary files
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rmdir(self, os.path.join(self.package_folder, "share"))

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "HPX")
        self.cpp_info.set_property("cmake_target_name", "HPX::hpx")
        self.cpp_info.set_property("pkg_config_name", "hpx")

        # Main library
        self.cpp_info.libs = ["hpx"]

        # Add debug postfix if in debug mode
        if self.settings.build_type == "Debug":
            self.cpp_info.libs = [lib + "d" for lib in self.cpp_info.libs]

        # System libraries
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs.extend(["pthread", "rt", "dl"])
        elif self.settings.os == "Windows":
            self.cpp_info.system_libs.extend(["ws2_32", "mswsock", "iphlpapi"])

        # Compiler flags
        if self.settings.os == "Linux":
            self.cpp_info.cxxflags.append("-pthread")

        # Define HPX_APPLICATION_EXPORTS for applications using HPX
        self.cpp_info.defines.append("HPX_APPLICATION_EXPORTS")

        # Set binary directory for HPX tools
        self.cpp_info.bindirs = ["bin"]

        # CMake module path for HPX CMake utilities
        self.cpp_info.builddirs.append(os.path.join("lib", "cmake", "HPX"))
