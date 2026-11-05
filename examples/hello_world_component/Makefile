# Copyright (c) 2014 Thomas Heller
#
# SPDX-License-Identifier: BSL-1.0
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt

.PHONY: all

ifeq ($(BUILD_TYPE),Debug)
  debug_suffix = d
  hpx_application = hpx_application_debug
  hpx_component = hpx_component_debug
else
  debug_suffix =
  hpx_application = hpx_application_release
  hpx_component = hpx_component_release
endif

APP_CFLAGS = $(CXX_FLAGS)
APP_CFLAGS += $(shell PKG_CONFIG_PATH=$(HPX_DIR) $(PKG_CONFIG) --cflags --libs $(hpx_application))
APP_CFLAGS += -lhpx_iostreams$(debug_suffix)
APP_CFLAGS += -DHPX_APPLICATION_NAME=test

COMP_CFLAGS = $(CXX_FLAGS)
COMP_CFLAGS += $(shell PKG_CONFIG_PATH=$(HPX_DIR) $(PKG_CONFIG) --cflags --libs $(hpx_component))
COMP_CFLAGS += -lhpx_iostreams$(debug_suffix)
COMP_CFLAGS += -DHPX_COMPONENT_NAME=test_component

ifneq ($(wildcard $(HPX_DIR)/*),)
all: hello_world_client
else
all:
	@echo "HPX_DIR=$(HPX_DIR) does not exist. Did you forget to run the install rule?"
endif

hello_world_client: $(SRC_DIR)/hello_world_client.cpp libhello_world_component.so
	$(CXX) -o hello_world_client -I$(SRC_DIR)/ $(SRC_DIR)/hello_world_client.cpp $(APP_CFLAGS) libhello_world_component.so

libhello_world_component.so: $(SRC_DIR)/hello_world_component.cpp
	$(CXX) -o libhello_world_component.so -fPIC -I$(SRC_DIR)/ $(SRC_DIR)/hello_world_component.cpp $(COMP_CFLAGS)
