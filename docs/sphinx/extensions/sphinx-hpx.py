# Copyright (c) 2018 Mikael Simberg
# Copyright (c) 2022-2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

import json
import os
from docutils import nodes
from sphinx.roles import ReferenceRole
from sphinx import addnodes


_symbol_anchors_cache = {}

def _load_symbol_anchors(app):
    """Load api/symbol_anchors.json (populated by
    cmake/templates/conf.py.in), mapping a \\page-tagged symbol title
    (e.g. "hpx::async") to the anchor of the (possibly disambiguated)
    page documenting it."""
    srcdir = app.srcdir
    if srcdir not in _symbol_anchors_cache:
        manifest = os.path.join(srcdir, 'api', 'symbol_anchors.json')
        try:
            with open(manifest) as f:
                _symbol_anchors_cache[srcdir] = json.load(f)
        except (OSError, ValueError):
            _symbol_anchors_cache[srcdir] = {}
    return _symbol_anchors_cache[srcdir]




class HPXCppRole(ReferenceRole):
    """A `:hpx:<kind>:` role, e.g. `:hpx:func:`hpx::async``.

    Behaves exactly like the corresponding built-in `:cpp:<kind>:` role
    (i.e. keeps normal signature-aware C++ domain cross-referencing), but
    first consults api/symbol_anchors.json and, if the referenced symbol
    has a disambiguated "counterpart" page (see overload_counterparts in
    conf.py.in), redirects there instead of resolving through the C++
    domain. This subsumes what the old, kind-agnostic hpx-api role did,
    while keeping ordinary domain-aware resolution for everything else.
    """
    def __init__(self, cpp_reftype):
        # name of the 'cpp' domain role this stands in for, e.g. 'func',
        # 'class', 'member', 'type', 'enum', 'concept'.
        super().__init__()
        self.cpp_reftype = cpp_reftype

    def run(self):
        target = self.target.lstrip('~')
        anchor = _load_symbol_anchors(self.env.app).get(target)
        if anchor is None:
            # No disambiguated counterpart page for this symbol: delegate
            # straight to the real :cpp:<kind>: role so behavior matches
            # exactly.
            cpp_role = self.env.get_domain('cpp').roles[self.cpp_reftype]
            return cpp_role('cpp:' + self.cpp_reftype, self.rawtext,
                self.text, self.lineno, self.inliner, self.options,
                self.content)

        title = self.title
        if not self.has_explicit_title and title[:1] == '~':
            title = title[1:]
            dot = title.rfind('::')
            if dot != -1:
                title = title[dot + 2:]

        refnode = addnodes.pending_xref(
            self.rawtext,
            refdomain='std',
            reftype='ref',
            reftarget=anchor,
            refexplicit=True,
            refwarn=True,
        )
        refnode += nodes.inline(self.rawtext, title, classes=['xref', 'std', 'std-ref'])
        return [refnode], []


# 'cpp' domain role kinds exposed as disambiguation-aware :hpx:<kind>:
# roles (see HPXCppRole above), e.g. :hpx:func:, :hpx:class:, ...
_HPX_CPP_ROLE_KINDS = ('func', 'class', 'struct', 'member', 'type', 'enum', 'concept', 'var')


def setup(app):
    app.add_role('hpx-issue', autolink('https://github.com/TheHPXProject/hpx/issues/%s', "Issue #"))
    app.add_role('hpx-header', autolink_hpx_file('http://github.com/TheHPXProject/hpx/blob/%s/%s/%s'))
    app.add_role('hpx-pr', autolink('https://github.com/TheHPXProject/hpx/pull/%s', "PR #"))
    app.add_role('cppreference-header', autolink('http://en.cppreference.com/w/cpp/header/%s'))
    app.add_role('cppreference-algorithm', autolink('http://en.cppreference.com/w/cpp/algorithm/%s'))
    app.add_role('cppreference-memory', autolink('http://en.cppreference.com/w/cpp/memory/%s'))
    app.add_role('cppreference-container', autolink('http://en.cppreference.com/w/cpp/container/%s'))
    app.add_role('cppreference-generic', autolink_generic('http://en.cppreference.com/w/cpp/%s/%s'))
    for kind in _HPX_CPP_ROLE_KINDS:
        app.add_role('hpx:' + kind, HPXCppRole(kind))


def autolink(pattern, prefix=''):
    def role(name, rawtext, text, lineno, inliner, options={}, content=[]):
        url = pattern % (text,)
        node = nodes.reference(rawtext, prefix + text, refuri=url, **options)
        return [node], []
    return role

# The text in the rst file should be:
# :hpx-header:`base_path,file_name`
def autolink_hpx_file(pattern):
    def role(name, rawtext, text, lineno, inliner, options={}, content=[]):
        text_parts = [p.strip() for p in text.split(',')]
        commit = inliner.document.settings.env.app.config.html_context['fullcommit']
        if len(text_parts) >= 2:
            url = pattern % (commit, text_parts[0], text_parts[1])
        else:
            url = pattern % (commit, text_parts[0], text_parts[0])
        node = nodes.reference(rawtext, text_parts[1], refuri=url, **options)
        return [node], []
    return role

# The text in the rst file should be:
# :cppreference-generic:`base_path,typename[,shown]`, for instance `thread,barrier`
def autolink_generic(pattern):
    def role(name, rawtext, text, lineno, inliner, options={}, content=[]):
        text_parts = [p.strip() for p in text.split(',')]
        shown_text = None
        if len(text_parts) >= 3:
            shown_text = text_parts[2]
            url = pattern % (text_parts[0], text_parts[1])
        elif len(text_parts) == 2:
            shown_text = text_parts[1]
            url = pattern % (text_parts[0], text_parts[1])
        else:
            shown_text = text_parts[0]
            url = pattern % (text_parts[0], text_parts[0])
        node = nodes.reference(rawtext, "std::" + shown_text, refuri=url, **options)
        return [node], []
    return role
