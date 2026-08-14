# Copyright (c) 2026 Arpit Singh
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

param(
    [Parameter(Mandatory = $true)]
    [string]$Path
)

$resolved_path = Resolve-Path -Path $Path -ErrorAction Stop
$exclude_targets = Get-Content -Path $resolved_path |
    Where-Object { $_ -notmatch '^\s*(#|$)' } |
    ForEach-Object { $_.Trim() }

[System.String]::Join('|', $exclude_targets)
