# Copyright (c) 2016 Marcin Copik
# Copyright (c) 2016 Parsa Amini
#
# SPDX-License-Identifier: BSL-1.0
#
#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

import re

compilation_database_dir = "@PROJECT_BINARY_DIR@"

def FlagsForFile(filename, **kwargs):

    a = ''
    #Ignore lines not containing CXX command
    with open('{0}/compile_commands.json'.format(compilation_database_dir), 'r') as f:
        a = ''.join([l.rstrip('\n') for l in f.readlines() if 'command' in l])

    final_flags = list(set(re.findall('-(?:std|W|w)\S+|-(?:I|include|isystem)(?:\S+| \S+)', a)))

    return {
        'flags': final_flags,
        'do_cache': True
    }

