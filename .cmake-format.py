# Copyright (c) 2020-2023 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# This cmake-format configuration file is a suggested configuration file for
# formatting CMake files for the HPX project.

# PLEASE NOTE: This file has been created and tested with cmake-format V0.6.10

# -----------------------------
# Options affecting formatting.
# -----------------------------
with section("format"):

  # If true, separate function names from parentheses with a space
  separate_fn_name_with_space = False

  # Format command names consistently as 'lower' or 'upper' case
  command_case = u'lower'

  # If the statement spelling length (including space and parenthesis) is
  # larger
  # than the tab width by more than this amount, then force reject un-nested
  # layouts.
  max_prefix_chars = 10

  # If the trailing parenthesis must be 'dangled' on its own line, then align
  # it
  # to this reference: `prefix`: the start of the statement, `prefix-indent`:
  # the start of the statement, plus one indentation level, `child`: align to
  # the column of the arguments
  dangle_align = u'prefix'

  # If an argument group contains more than this many sub-groups (parg or kwarg
  # groups) then force it to a vertical layout.
  max_subgroups_hwrap = 2

  # If the statement spelling length (including space and parenthesis) is
  # smaller than this amount, then force reject nested layouts.
  min_prefix_chars = 4

  # If a positional argument group contains more than this many arguments, then
  # force it to a vertical layout.
  max_pargs_hwrap = 6

  # If a candidate layout is wrapped horizontally but it exceeds this many
  # lines, then reject the layout.
  max_lines_hwrap = 2

  # If true, the parsers may infer whether or not an argument list is sortable
  # (without annotation).
  autosort = False

  # What style line endings to use in the output.
  line_ending = u'auto'

  # How wide to allow formatted cmake files
  line_width = 80

  # If a statement is wrapped to more than one line, than dangle the closing
  # parenthesis on its own line.
  dangle_parens = True

  # How many spaces to tab for indent
  tab_size = 2

  # A list of command names which should always be wrapped
  always_wrap = []

  # If true, separate flow control names from their parentheses with a space
  separate_ctrl_name_with_space = False

  # If a cmdline positional group consumes more than this many lines without
  # nesting, then invalidate the layout (and nest)
  max_rows_cmdline = 2

  # By default, if cmake-format cannot successfully fit everything into the
  # desired linewidth it will apply the last, most aggressive attempt that it
  # made.  If this flag is True, however, cmake-format will print error, exit
  # with non-zero status code, and write-out nothing
  require_valid_layout = False

  # Format keywords consistently as 'lower' or 'upper' case
  keyword_case = u'unchanged'

  # If true, the argument lists which are known to be sortable will be sorted
  # lexicographicall
  enable_sort = True

  # A dictionary mapping layout nodes to a list of wrap decisions.  See the
  # documentation for more information.
  layout_passes = {}

# ------------------------------------------------
# Options affecting comment reflow and formatting.
# ------------------------------------------------
with section("markup"):

  # If comment markup is enabled, don't reflow any comment block which matches
  # this (regex) pattern.  Default is `None` (disabled).
  literal_comment_pattern = None

  # If a comment line starts with at least this many consecutive hash
  # characters, then don't lstrip() them off.  This allows for lazy hash rulers
  # where the first hash char is not separated by space
  hashruler_min_length = 10

  # Regular expression to match preformat fences in comments default=
  # ``r'^\s*([`~]{3}[`~]*)(.*)$'``
  fence_pattern = u'^\\s*([`~]{3}[`~]*)(.*)$'

  # If true, then insert a space between the first hash char and remaining hash
  # chars in a hash ruler, and normalize its length to fill the column
  canonicalize_hashrulers = True

  # If a comment line matches starts with this pattern then it is explicitly a
  # trailing comment for the preceding argument.  Default is '#<'
  explicit_trailing_pattern = u'#<'

  # If comment markup is enabled, don't reflow the first comment block in each
  # listfile.  Use this to preserve formatting of your copyright/license
  # statements.
  first_comment_is_literal = True

  # enable comment markup parsing and reflow
  enable_markup = True

  # Regular expression to match rulers in comments default=
  # ``r'^\s*[^\w\s]{3}.*[^\w\s]{3}$'``
  ruler_pattern = u'^\\s*[^\\w\\s]{3}.*[^\\w\\s]{3}$'

  # What character to use as punctuation after numerals in an enumerated list
  enum_char = u'.'

  # What character to use for bulleted lists
  bullet_char = u'*'

# ----------------------------
# Options affecting the linter
# ----------------------------
with section("lint"):

  # regular expression pattern describing valid function names
  function_pattern = u'[0-9a-z_]+'

  # regular expression pattern describing valid names for function/macro
  # arguments and loop variables.
  argument_var_pattern = u'[a-z][a-z0-9_]+'

  # a list of lint codes to disable
  disabled_codes = []

  # Require at least this many newlines between statements
  min_statement_spacing = 1

  # regular expression pattern describing valid macro names
  macro_pattern = u'[0-9A-Z_]+'

  # regular expression pattern describing valid names for public directory
  # variables
  public_var_pattern = u'[A-Z][0-9A-Z_]+'
  max_statements = 50

  # In the heuristic for C0201, how many conditionals to match within a loop in
  # before considering the loop a parser.
  max_conditionals_custom_parser = 2

  # regular expression pattern describing valid names for variables with global
  # (cache) scope
  global_var_pattern = u'[A-Z][0-9A-Z_]+'

  # regular expression pattern describing valid names for keywords used in
  # functions or macros
  keyword_pattern = u'[A-Z][0-9A-Z_]+'
  max_arguments = 5

  # regular expression pattern describing valid names for privatedirectory
  # variables
  private_var_pattern = u'_[0-9a-z_]+'
  max_localvars = 15
  max_branches = 12

  # regular expression pattern describing valid names for variables with local
  # scope
  local_var_pattern = u'[a-z][a-z0-9_]+'

  # Require no more than this many newlines between statements
  max_statement_spacing = 2

  # regular expression pattern describing valid names for variables with global
  # scope (but internal semantic)
  internal_var_pattern = u'_[A-Z][0-9A-Z_]+'
  max_returns = 6

# -------------------------------------
# Miscellaneous configurations options.
# -------------------------------------
with section("misc"):

  # A dictionary containing any per-command configuration overrides.  Currently
  # only `command_case` is supported.
  per_command = { }

# ----------------------------------
# Options affecting listfile parsing
# ----------------------------------
with section("parse"):

  # Specify structure for custom cmake functions
  # (the body of this structure was generated using
  #     'cmake-genparsers -f python cmake/HPX*.cmake'
  #
  additional_commands = {
   'add_hpx_compile_test': { 'kwargs': { 'COMPONENT_DEPENDENCIES': '+',
                                          'DEPENDENCIES': '+',
                                          'FOLDER': 1,
                                          'SOURCES': '+',
                                          'SOURCE_ROOT': 1},
                              'pargs': { 'flags': ['FAILURE_EXPECTED', 'NOLIBS'],
                                         'nargs': '2+'}},
    'add_hpx_compile_test_target_dependencies': {
                              'kwargs': { 'COMPONENT_DEPENDENCIES': '+',
                                          'DEPENDENCIES': '+',
                                          'FOLDER': 1,
                                          'SOURCES': '+',
                                          'SOURCE_ROOT': 1},
                              'pargs': { 'flags': ['FAILURE_EXPECTED', 'NOLIBS'],
                                         'nargs': '2+'}},
    'add_hpx_component': { 'kwargs': { 'AUXILIARY': '+',
                                       'COMPILE_FLAGS': '+',
                                       'COMPONENT_DEPENDENCIES': '+',
                                       'DEPENDENCIES': '+',
                                       'FOLDER': 1,
                                       'HEADERS': '+',
                                       'HEADER_GLOB': 1,
                                       'HEADER_ROOT': 1,
                                       'INI': 1,
                                       'INSTALL_SUFFIX': 1,
                                       'INSTALL_COMPONENT': 1,
                                       'LANGUAGE': 1,
                                       'LINK_FLAGS': '+',
                                       'OUTPUT_SUFFIX': 1,
                                       'SOURCES': '+',
                                       'SOURCE_GLOB': 1,
                                       'SOURCE_ROOT': 1,
                                       'SCAN_FOR_MODULES': 1},
                           'pargs': { 'flags': ['EXCLUDE_FROM_ALL',
                                                 'INSTALL_HEADERS',
                                                 'INTERNAL_FLAGS',
                                                 'NOEXPORT',
                                                 'AUTOGLOB',
                                                 'STATIC',
                                                 'PLUGIN',
                                                 'PREPEND_SOURCE_ROOT',
                                                 'PREPEND_HEADER_ROOT',
                                                 'UNITY_BUILD'],
                                      'nargs': '1+'}},
    'add_hpx_config_test': { 'kwargs': { 'ARGS': '+',
                                         'CMAKECXXFEATURE': 1,
                                         'COMPILE_DEFINITIONS': '+',
                                         'DEFINITIONS': '+',
                                         'INCLUDE_DIRECTORIES': '+',
                                         'LIBRARIES': '+',
                                         'LINK_DIRECTORIES': '+',
                                         'REQUIRED': '+',
                                         'ROOT': 1,
                                         'SOURCE': 1},
                             'pargs': { 'flags': ['FILE', 'EXECUTE'],
                                        'nargs': '1+'}},
    'add_hpx_example_target_dependencies': { 'kwargs': {},
                                             'pargs': { 'flags': ['DEPS_ONLY'],
                                                        'nargs': '2+'}},
    'add_hpx_example_test': {'pargs': {'nargs': 2}},
    'add_hpx_executable': { 'kwargs': { 'AUXILIARY': '+',
                                        'COMPILE_FLAGS': '+',
                                        'COMPONENT_DEPENDENCIES': '+',
                                        'DEPENDENCIES': '+',
                                        'FOLDER': 1,
                                        'HEADERS': '+',
                                        'HEADER_GLOB': 1,
                                        'HEADER_ROOT': 1,
                                        'HPX_PREFIX': 1,
                                        'SCAN_FOR_MODULES': 1,
                                        'INI': 1,
                                        'INSTALL_SUFFIX': 1,
                                        'LANGUAGE': 1,
                                        'LINK_FLAGS': '+',
                                        'OUTPUT_SUFFIX': 1,
                                        'SOURCES': '+',
                                        'SOURCE_GLOB': 1,
                                        'SOURCE_ROOT': 1},
                            'pargs': { 'flags': ['EXCLUDE_FROM_ALL',
                                                  'EXCLUDE_FROM_DEFAULT_BUILD',
                                                  'AUTOGLOB',
                                                  'INTERNAL_FLAGS',
                                                  'NOLIBS',
                                                  'NOHPX_INIT'],
                                       'nargs': '1+'}},
    'add_hpx_header_tests': { 'kwargs': { 'COMPONENT_DEPENDENCIES': '+',
                                          'DEPENDENCIES': '+',
                                          'EXCLUDE': '+',
                                          'EXCLUDE_FROM_ALL': '+',
                                          'HEADERS': '+',
                                          'HEADER_ROOT': 1},
                              'pargs': {'flags': ['NOLIBS'], 'nargs': '1+'}},
    'add_hpx_headers_compile_test': {
                              'kwargs': { 'COMPONENT_DEPENDENCIES': '+',
                                          'DEPENDENCIES': '+',
                                          'FOLDER': 1,
                                          'SOURCES': '+',
                                          'SOURCE_ROOT': 1},
                              'pargs': { 'flags': ['FAILURE_EXPECTED', 'NOLIBS'],
                                         'nargs': '2+'}},
    'add_hpx_library': { 'kwargs': { 'AUXILIARY': '+',
                                     'COMPILER_FLAGS': '+',
                                     'COMPONENT_DEPENDENCIES': '+',
                                     'DEPENDENCIES': '+',
                                     'FOLDER': 1,
                                     'HEADERS': '+',
                                     'HEADER_GLOB': 1,
                                     'HEADER_ROOT': 1,
                                     'INSTALL_SUFFIX': 1,
                                     'LINK_FLAGS': '+',
                                     'OUTPUT_SUFFIX': 1,
                                     'SOURCES': '+',
                                     'SOURCE_GLOB': 1,
                                     'SOURCE_ROOT': 1,
                                     'SCAN_FOR_MODULES': 1},
                         'pargs': { 'flags': ['EXCLUDE_FROM_ALL',
                                               'INSTALL_HEADERS',
                                               'INTERNAL_FLAGS',
                                               'NOLIBS',
                                               'NOEXPORT',
                                               'AUTOGLOB',
                                               'STATIC',
                                               'PLUGIN',
                                               'NONAMEPREFIX',
                                               'PREPEND_SOURCE_ROOT',
                                               'PREPEND_HEADER_ROOT',
                                               'UNITY_BUILD'],
                                    'nargs': '1+'}},
    'add_hpx_library_headers': { 'kwargs': {'EXCLUDE': '+', 'GLOBS': '+'},
                                 'pargs': {'flags': ['APPEND'], 'nargs': '2+'}},
    'add_hpx_library_headers_noglob': { 'kwargs': { 'EXCLUDE': '+',
                                                    'HEADERS': '+'},
                                        'pargs': { 'flags': ['APPEND'],
                                                   'nargs': '1+'}},
    'add_hpx_library_sources': { 'kwargs': {'EXCLUDE': '+', 'GLOBS': '+'},
                                 'pargs': {'flags': ['APPEND'], 'nargs': '2+'}},
    'add_hpx_library_sources_noglob': { 'kwargs': { 'EXCLUDE': '+',
                                                    'SOURCES': '+'},
                                        'pargs': { 'flags': ['APPEND'],
                                                   'nargs': '1+'}},
    'add_hpx_module': { 'kwargs': { 'CMAKE_SUBDIRS': '+',
                                    'COMPAT_HEADERS': '+',
                                    'GENERATED_HEADERS': '+',
                                    'DEPENDENCIES': '+',
                                    'PRIVATE_DEPENDENCIES': '+',
                                    'INCLUDE_DIRECTORIES': '+',
                                    'EXCLUDE_FROM_GLOBAL_HEADER': '+',
                                    'ADD_TO_GLOBAL_HEADER': '+',
                                    'GLOBAL_HEADER_GEN': 1,
                                    'GLOBAL_HEADER_MODULE_GEN': 1,
                                    'MODULE_SOURCE': 1,
                                    'HEADERS': '+',
                                    'MACRO_HEADERS': '+',
                                    'OBJECTS': '+',
                                    'MODULE_DEPENDENCIES': '+',
                                    'SOURCES': '+'},
                        'pargs': { 'flags': ['CUDA',
                                             'CONFIG_FILES',
                                             'NO_CONFIG_IN_GENERATED_HEADERS'],
                                   'nargs': '1+'}},
    'add_hpx_performance_test': {'pargs': {'nargs': 2}},
    'add_hpx_pseudo_dependencies': {'pargs': {'nargs': 0}},
    'add_hpx_pseudo_dependencies_no_shortening': {'pargs': {'nargs': 0}},
    'add_hpx_pseudo_target': {'pargs': {'nargs': 0}},
    'add_hpx_regression_compile_test': {
                              'kwargs': { 'COMPONENT_DEPENDENCIES': '+',
                                          'DEPENDENCIES': '+',
                                          'FOLDER': 1,
                                          'SOURCES': '+',
                                          'SOURCE_ROOT': 1},
                              'pargs': { 'flags': ['FAILURE_EXPECTED', 'NOLIBS'],
                                         'nargs': '2+'}},
    'add_hpx_regression_test': { 'kwargs': { 'COMPONENT_DEPENDENCIES': '+',
                                             'DEPENDENCIES': '+',
                                             'EXECUTABLE': 1,
                                             'FOLDER': 1,
                                             'SOURCES': '+',
                                             'SOURCE_ROOT': 1,
                                             'ARGS': '+',
                                             'EXECUTABLE': 1,
                                             'PSEUDO_DEPS_NAME': 1,
                                             'LOCALITIES': 1,
                                             'PARCELPORTS': '+',
                                             'THREADS_PER_LOCALITY': 1,
                                             'TIMEOUT': 1},
                              'pargs': { 'flags': ['FAILURE_EXPECTED',
                                                   'NOLIBS',
                                                   'RUN_SERIAL',
                                                   'NO_PARCELPORT_TCP',
                                                   'NO_PARCELPORT_LCI',
                                                   'NO_PARCELPORT_MPI',
                                                   'NO_PARCELPORT_GASNET'],
                                         'nargs': '2+'}},
    'add_hpx_source_group': { 'kwargs': { 'CLASS': 1,
                                          'NAME': 1,
                                          'ROOT': 1,
                                          'TARGETS': '+'},
                              'pargs': {'flags': [], 'nargs': '*'}},
    'add_hpx_test': { 'kwargs': { 'ARGS': '+',
                                  'EXECUTABLE': 1,
                                  'LOCALITIES': 1,
                                  'PARCELPORTS': '+',
                                  'THREADS_PER_LOCALITY': 1},
                      'pargs': {'flags': ['FAILURE_EXPECTED',
                                          'RUN_SERIAL',
                                          'NO_PARCELPORT_TCP',
                                          'NO_PARCELPORT_LCI',
                                          'NO_PARCELPORT_MPI'],
                                'nargs': '2+'}},
    'add_hpx_test_target_dependencies': { 'kwargs': {'PSEUDO_DEPS_NAME': 1},
                                          'pargs': {'flags': [], 'nargs': '2+'}},
    'add_hpx_unit_compile_test': {
                              'kwargs': { 'COMPONENT_DEPENDENCIES': '+',
                                          'DEPENDENCIES': '+',
                                          'FOLDER': 1,
                                          'SOURCES': '+',
                                          'SOURCE_ROOT': 1},
                              'pargs': { 'flags': ['FAILURE_EXPECTED', 'NOLIBS'],
                                         'nargs': '2+'}},
    'add_hpx_unit_test': { 'kwargs': { 'COMPONENT_DEPENDENCIES': '+',
                                          'DEPENDENCIES': '+',
                                          'FOLDER': 1,
                                          'SOURCES': '+',
                                          'SOURCE_ROOT': 1,
                                          'ARGS': '+',
                                          'EXECUTABLE': 1,
                                          'PSEUDO_DEPS_NAME': 1,
                                          'LOCALITIES': 1,
                                          'PARCELPORTS': '+',
                                          'THREADS_PER_LOCALITY': 1},
                           'pargs': { 'flags': ['FAILURE_EXPECTED',
                                                'NOLIBS',
                                                'RUN_SERIAL',
                                                'NO_PARCELPORT_TCP',
                                                'NO_PARCELPORT_LCI',
                                                'NO_PARCELPORT_MPI'],
                                      'nargs': '2+'}},
    'add_parcelport': { 'kwargs': { 'COMPILE_FLAGS': '+',
                                    'DEPENDENCIES': '+',
                                    'FOLDER': 1,
                                    'HEADERS': '+',
                                    'INCLUDE_DIRS': '+',
                                    'LINK_FLAGS': '+',
                                    'SOURCES': '+'},
                        'pargs': {'flags': ['STATIC', 'EXPORT'], 'nargs': '1+'}},
    'add_test_and_deps_compile_test': {
                              'kwargs': { 'COMPONENT_DEPENDENCIES': '+',
                                          'DEPENDENCIES': '+',
                                          'FOLDER': 1,
                                          'SOURCES': '+',
                                          'SOURCE_ROOT': 1},
                              'pargs': { 'flags': ['FAILURE_EXPECTED', 'NOLIBS'],
                                         'nargs': '3+'}},
    'add_test_and_deps_test': {'pargs': {'nargs': 3}},
    'create_configuration_summary': {'pargs': {'nargs': 2}},
    'create_symbolic_link': {'pargs': {'nargs': 2}},
    'get_target_property': {'pargs': {'nargs': 3}},
    'hpx_add_compile_flag': {'pargs': {'nargs': 0}},
    'hpx_add_compile_flag_if_available': { 'kwargs': { 'CONFIGURATIONS': '+',
                                                       'LANGUAGES': '+',
                                                       'NAME': 1},
                                           'pargs': {'flags': [], 'nargs': '1+'}},
    'hpx_add_config_cond_define': {'pargs': {'nargs': 1}},
    'hpx_add_config_define': {'pargs': {'nargs': 1}},
    'hpx_add_config_define_namespace': { 'kwargs': { 'DEFINE': 1,
                                                     'NAMESPACE': 1,
                                                     'VALUE': '+'},
                                         'pargs': {'flags': [], 'nargs': '*'}},
    'hpx_add_link_flag': { 'kwargs': {'CONFIGURATIONS': '+', 'TARGETS': '+'},
                           'pargs': {'flags': [], 'nargs': '1+'}},
    'hpx_add_link_flag_if_available': { 'kwargs': {'NAME': 1, 'TARGETS': '+'},
                                        'pargs': {'flags': [], 'nargs': '1+'}},
    'hpx_add_target_compile_definition': { 'kwargs': {'CONFIGURATIONS': '+'},
                                           'pargs': { 'flags': ['PUBLIC'],
                                                      'nargs': '1+'}},
    'hpx_add_target_compile_option': { 'kwargs': { 'CONFIGURATIONS': '+',
                                                   'LANGUAGES': '+'},
                                       'pargs': { 'flags': ['PUBLIC'],
                                                  'nargs': '1+'}},
    'hpx_add_target_compile_option_if_available': { 'kwargs': { 'CONFIGURATIONS': '+',
                                                                'LANGUAGES': '+',
                                                                'NAME': 1,
                                                                'RESULT': 1},
                                                    'pargs': { 'flags': ['PUBLIC', 'PRIVATE'],
                                                               'nargs': '1+'}},
    'hpx_append_property': {'pargs': {'nargs': 2}},
    'hpx_check_for_builtin_integer_pack': {'pargs': {'nargs': 0}},
    'hpx_check_for_builtin_make_integer_seq': {'pargs': {'nargs': 0}},
    'hpx_check_for_builtin_type_pack_element': {'pargs': {'nargs': 0}},
    'hpx_check_for_cxx11_std_atomic': {'pargs': {'nargs': 0}},
    'hpx_check_for_cxx11_std_atomic_128bit': {'pargs': {'nargs': 0}},
    'hpx_check_for_cxx11_std_quick_exit': {'pargs': {'nargs': 0}},
    'hpx_check_for_cxx17_aligned_new': {'pargs': {'nargs': 0}},
    'hpx_check_for_cxx17_filesystem': {'pargs': {'nargs': 0}},
    'hpx_check_for_cxx17_hardware_destructive_interference_size': { 'pargs': { 'nargs': 0}},
    'hpx_check_for_libfun_std_experimental_optional': {'pargs': {'nargs': 0}},
    'hpx_check_for_mm_prefetch': {'pargs': {'nargs': 0}},
    'hpx_check_for_stable_inplace_merge': {'pargs': {'nargs': 0}},
    'hpx_check_for_unistd_h': {'pargs': {'nargs': 0}},
    'hpx_collect_usage_requirements': { 'kwargs': {'EXCLUDE': '+'},
                                        'pargs': {'flags': [], 'nargs': '10+'}},
    'hpx_config_loglevel': {'pargs': {'nargs': 2}},
    'hpx_construct_cflag_list': {'pargs': {'nargs': 6}},
    'hpx_construct_library_list': {'pargs': {'nargs': 3}},
    'hpx_cpuid': {'pargs': {'nargs': 2}},
    'hpx_debug': {'pargs': {'nargs': 0}},
    'hpx_error': {'pargs': {'nargs': 0}},
    'hpx_export_modules_targets': {'pargs': {'nargs': 0}},
    'hpx_export_targets': {'pargs': {'nargs': 0}},
    'hpx_force_out_of_tree_build': {'pargs': {'nargs': 1}},
    'hpx_generate_pkgconfig_from_target': { 'kwargs': {'EXCLUDE': '+'},
                                            'pargs': { 'flags': [],
                                                       'nargs': '3+'}},
    'hpx_handle_component_dependencies': {'pargs': {'nargs': 1}},
    'hpx_include': {'pargs': {'nargs': 0}},
    'hpx_info': {'pargs': {'nargs': 0}},
    'hpx_message': {'pargs': {'nargs': 1}},
    'hpx_option': { 'kwargs': {'CATEGORY': 1, 'MODULE': 1, 'STRINGS': '+'},
                    'pargs': {'flags': ['ADVANCED'], 'nargs': '4+'}},
    'hpx_perform_cxx_feature_tests': {'pargs': {'nargs': 0}},
    'hpx_print_list': {'pargs': {'nargs': 3}},
    'hpx_remove_link_flag': { 'kwargs': {'CONFIGURATIONS': '+', 'TARGETS': '+'},
                              'pargs': {'flags': [], 'nargs': '1+'}},
    'hpx_remove_target_compile_option': { 'kwargs': {'CONFIGURATIONS': '+'},
                                          'pargs': { 'flags': ['PUBLIC'],
                                                     'nargs': '1+'}},
    'hpx_sanitize_usage_requirements': {'pargs': {'nargs': 2}},
    'hpx_set_cmake_policy': {'pargs': {'nargs': 2}},
    'hpx_set_lib_name': {'pargs': {'nargs': 2}},
    'hpx_set_option': { 'kwargs': {'HELPSTRING': 1, 'TYPE': 1, 'VALUE': 1},
                        'pargs': {'flags': ['FORCE'], 'nargs': '1+'}},
    'hpx_setup_target': { 'kwargs': { 'COMPILE_FLAGS': '+',
                                      'COMPONENT_DEPENDENCIES': '+',
                                      'DEPENDENCIES': '+',
                                      'FOLDER': 1,
                                      'HEADER_ROOT': 1,
                                      'HPX_PREFIX': 1,
                                      'INSTALL_FLAGS': '+',
                                      'INSTALL_PDB': '+',
                                      'LINK_FLAGS': '+',
                                      'NAME': 1,
                                      'SOVERSION': 1,
                                      'TYPE': 1,
                                      'SCAN_FOR_MODULES': 1,
                                      'VERSION': 1},
                          'pargs': { 'flags': ['EXPORT',
                                                'INSTALL',
                                                'INSTALL_HEADERS',
                                                'INTERNAL_FLAGS',
                                                'NOLIBS',
                                                'PLUGIN',
                                                'NONAMEPREFIX',
                                                'NOTLLKEYWORD'],
                                     'nargs': '1+'}},
    'hpx_source_to_doxygen': { 'kwargs': { 'DEPENDENCIES': '+',
                                           'DOXYGEN_ARGS': '+'},
                               'pargs': {'flags': [], 'nargs': '1+'}},
    'hpx_warn': {'pargs': {'nargs': 0}},
    'hpx_setup_mpi': {'pargs': {'nargs': 0}},
    'shorten_hpx_pseudo_target': {'pargs': {'nargs': 2}},
    'write_config_defines_file': { 'kwargs': { 'FILENAME': 1,
                                               'NAMESPACE': 1,
                                               'TEMPLATE': 1},
                                   'pargs': {'flags': [], 'nargs': '*'}},
    'hpx_collect_std_headers': { 'kwargs': { 'SOURCES': '+',
                                             'SOURCE_ROOT': 1,
                                             'GENERATED_ROOT': 1,
                                             'FOUND_HEADERS': 1},
                          'pargs': { 'flags': [], 'nargs': '1+'}},
    'hpx_extract_includes_from_file': { 'kwargs': { 'SOURCE': 1,
                                                    'INCLUDE_DIRS': '+',
                                                    'FOUND_HEADERS': 1},
                          'pargs': { 'flags': [], 'nargs': '1+'}},
    'hpx_configure_module_producer': { 'kwargs': {},
                          'pargs': { 'flags': [], 'nargs': '1+'}}
  }

  # Specify property tags.
  proptags = []

  # Specify variable tags.
  vartags = []

# -------------------------------
# Options affecting file encoding
# -------------------------------
with section("encode"):

  # If true, emit the unicode byte-order mark (BOM) at the start of the file
  emit_byteorder_mark = False

  # Specify the encoding of the input file.  Defaults to utf-8
  input_encoding = u'utf-8'

  # Specify the encoding of the output file.  Defaults to utf-8.  Note that
  # cmake
  # only claims to support utf-8 so be careful when using anything else
  output_encoding = u'utf-8'

