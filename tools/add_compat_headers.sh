#!/usr/bin/env bash

# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Script to introduce the deprecated include after factorazing in modules
# This file is globbing through the include_compatibility folder, so basic 
# files should already be here

# There is a possibility to specify the files manually

function extra_usage_message() {
    echo
    echo "- Can specify the --project_path if different from the environmental"
    echo "variable \$HPX_ROOT"
    echo "- Can also specify some target files if no globbing (without any extension) with:"
    echo "--files \"<filename1> <filename2>\""
    echo "Example with files: $0 -m module --files file -o hpx/util -n hpx/module -p \$PWD"
}

if [[ $# -lt 1 ]]; then
    arg=${BASH_SOURCE[0]}
    echo "Usage : \"$arg\" -m <module_name> --old_path <include_path> --new_path <include_path>"
    echo "Example: \"$arg\" -m cache -o hpx/util/cache -n hpx/cache"
    extra_usage_message
    exit
fi

function parse_arguments() {

    # store arguments list
    POSITIONAL=()

    while [[ $# -gt 0 ]]
    do
        local key="$1"
        case $key in
            -f|--files)
                all_files=0
                files=$2
                echo "manually specified files => no globbing"
                shift # pass option
                shift # pass value
                ;;
            -m|--module)
                module=$2
                echo "module : ${module}"
                shift # pass option
                shift # pass value
                ;;
            -n|--new_path)
                new_path_set=true
                new_path=$2
                echo "new_path : ${new_path}"
                shift # pass option
                shift # pass value
                ;;
            -o|--old_path)
                old_path_set=true
                old_path=$2
                echo "old_path : ${old_path}"
                shift # pass option
                shift # pass value
                ;;
            -p|--project_path)
                project_path=$2
                shift # pass option
                shift # pass value
                ;;
            --help|*)
                echo $"Usage: $0 [-m, --module <value>] [-o, --old_path <value>]"
                echo "[-n, --new_path <value>] [-p, --project_path <value>]"
                echo "[-f, --files \"<value1> <value2>\"]"
                echo "Example: "$0" -m cache -o hpx/util/cache -n hpx/cache"
                extra_usage_message
                exit
        esac
    done

    # restore positional parameters
    set -- "${POSITIONAL[@]}"

}

# Default values which can be overwritten while parsing args
project_path=$HPX_ROOT
module=cache
new_path_set=false
old_path_set=false
# Default is globbing compatibility files
all_files=1
files=

echo
# Parsing arguments
parse_arguments "$@"

echo "project_path: ${project_path}"
# Usual vars (depend on the parsing step)
libs_path=$project_path/libs
module_path=$libs_path/${module}

# Error handling
if [[ "$old_path_set" = "false" ]]; then
    old_path=hpx/util/${module}
fi
if [[ "$new_path_set" = "false" ]]; then
    new_path=hpx/${module}
fi
if [[ "$old_path_set" = "false" && $all_files -eq 0 ]]; then
    echo "Attention only the basename of the files should be specified"
fi
# Project path not set (full specified path to be sure which source is used)
if [[ -z $HPX_ROOT && -z $project_path ]]; then
    "HPX_ROOT env var doesn't exists and project_path option not specified !"
    exit
fi

pushd $module_path/include_compatibility > /dev/null
if [[ $? -eq 1 ]]; then
    echo -e "\e[31mPlease specify a correct project_path"
    exit
fi

# To enable **
shopt -s globstar
# Globbing step to get all the include_compatibility files, the files have to
# already be there, we are just rewriting them
if [[ all_files -eq 0 ]]; then
    files=($files)  # Make the string become an array
    files=(${files[@]/%/.hpp})
    files=(${files[@]/#/$old_path/})
else
    files=($(ls **/*.hpp))
fi

echo
echo -e "\e[32mFiles overwritten :"
# Create the corresponding compatibility headers
for full_file in "${files[@]}"; do
    # basename not used otherwise we lose hierarchy if any
    f=${full_file#"$old_path/"}
    echo $full_file

# No indentation otherwise it appears in the cat
cat >${full_file} <<EOL
//  Copyright (c) 2019 Ste||ar Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/${module}/config/defines.hpp>
#include <${new_path}/${f}>

#if HPX_HAVE_DEPRECATION_WARNINGS
#if defined(HPX_MSVC)
#pragma message( \\
    "The header ${old_path}/${f} is deprecated, \\
    please include ${new_path}/${f} instead")
#else
#warning \\
    "The header ${old_path}/${f} is deprecated, \\
    please include ${new_path}/${f} instead"
#endif
#endif
EOL

done

popd > /dev/null
