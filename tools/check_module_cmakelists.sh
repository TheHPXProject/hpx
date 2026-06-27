#!/bin/bash

# Copyright (c)      2020 STE||AR GROUP
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

function check_failure {
    string_to_grep=$1
    tmp_module=$2
    cmakelist=$3
    grep -Fq "$string_to_grep" $cmakelist
    if [[ $? -eq 1 ]]; then
        echo "Missing ${string_to_grep} in libs/${tmp_module}/CMakeLists.txt"
    fi
}

# Check the dependencies of the module listed in its CMakeLists.txt
function check_module_dependencies() {
    tmp_module=$1
    tmp_module_name=$(basename $tmp_module)
    non_module_files_list=$2
    tmp_group_list=$3

    # Find the dependencies through the includes and remove hpx/hpx_* like
    includes=($(grep -Erho 'hpx/modules/[_a-z]*\.hpp\>' --include=*.{hpp,cpp}\
        include src 2> /dev/null | sort | uniq | grep -v hpx/hpx))

    # Check if the dependency is inside the CMakeLists.txt
    for include in "${includes[@]}"
    do
        # Exclude the headers from the main hpx/ dir
        if [[ ! "${non_module_files_list[@]}" =~ "$(basename $include)" ]]; then
            # Isolate the name of the module from the include
            module_deps=$(basename $include | cut -d'.' -f1)
            # Check if the name is not the current module and check if it is
            # contained in the current module group
            if [[ ! "$module_deps" == "$tmp_module_name" ]]; then
                for group_module in ${tmp_group_list}; do
                    if [[ "$module_deps" == "$group_module" ]]; then
                        check_failure "hpx_${module_deps}" "$tmp_module" CMakeLists.txt
                        break
                    fi
                done
            fi
        fi
    done
}

function check_cmakelists_files() {
    tmp_module=$1
    tmp_dir=$2
    if [ -d "$tmp_dir" ]; then
        pushd "$tmp_dir" > /dev/null

            # Silence the .cpp not found errors in include dir and inversely
            module_files=($(ls **/*.{hpp,cpp} 2> /dev/null))
            # Check the presence of the header in the CMakeLists.txt of the module
            for header in "${module_files[@]}"
            do
                check_failure "$header" "$tmp_module" ../CMakeLists.txt
            done

        popd > /dev/null
    fi
}

########
# MAIN #
########

# Enable globbing
shopt -s globstar

# HPX source directory
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source_dir="$(cd "$script_dir/.." && pwd)"

# Where to write the dependencies output files
output_dir=/tmp
# Helper to filter out the dependencies from other groups
module_groups=(core full)

pushd "$source_dir/libs" > /dev/null

# Extract the list of the modules
modules_list=($(find . -mindepth 2 -maxdepth 2 -type d | sed "s|^\./||" | sort))

# Create a module list per module group
declare -A group_modules=()
for group in "${module_groups[@]}"
do
    group_modules[$group]="$(cd $group > /dev/null && find . -maxdepth 1 \
        -type d ! -path . | sed "s|\./||" && cd .. > /dev/null)"
done

# Construct a list for each of the module groups

# Find non module headers under the main hpx/ dir to exclude them later
non_module_files_list=()
if [ -d ../init/include/hpx ]; then
    non_module_files_list=($(ls ../init/include/hpx/*.hpp 2>/dev/null | xargs -n1 basename))
fi

echo "" > "$output_dir/missing_files.txt"
echo "" > "$output_dir/missing_deps.txt"

# Iterate on all modules of the libs/ dir
for module in "${modules_list[@]}"
do
    pushd "${module}" > /dev/null

        module_group=$(dirname $module)
        group_list=${group_modules[$module_group]}
        check_module_dependencies "$module" "$non_module_files_list" "${group_list[@]}" >> "$output_dir"/missing_deps.txt
        check_cmakelists_files "$module" include >> "$output_dir/missing_files.txt"
        check_cmakelists_files "$module" src >> "$output_dir/missing_files.txt"

    popd > /dev/null

done
