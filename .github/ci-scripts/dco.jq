# Copyright (c) 2026 The STE||AR Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Developer Certificate of Origin (DCO) check of the commits of a pull request.
# The input is the list of commits as returned by the GitHub API
# (repos/<owner>/<repo>/pulls/<number>/commits), the output is one line for each
# commit that fails the check (nothing if all commits pass).
#
# Merge commits and commits of bots are skipped. Every other commit needs a
# 'Signed-off-by: Name <email>' line naming its author or committer, and a
# valid author (or committer) email address.

.[]
| select((.parents | length) < 2 and .author.type != "Bot")
| ([.commit.author.name, .commit.committer.name] | map(ascii_downcase)) as $names
| ([.commit.author.email, .commit.committer.email] | map(ascii_downcase)) as $emails
| [.commit.message | split("\n")[]
   | capture("^Signed-off-by: (?<name>.*) <(?<email>.*)>$"; "i")] as $signoffs
| (.commit.author.email // .commit.committer.email) as $email
| if ($signoffs | length) == 0 then
    "\(.sha[0:10]) the sign-off is missing"
  elif ($email | test("^[^@ ]+@[^@ ]+[.][^@ ]+$") | not) then
    "\(.sha[0:10]) \($email) is not a valid email address"
  elif any($signoffs[];
      ((.name | ascii_downcase) as $n | $names | index($n)) and
      ((.email | ascii_downcase) as $e | $emails | index($e))) then
    empty
  else
    "\(.sha[0:10]) no sign-off matches \(.commit.author.name) <\(.commit.author.email)>"
  end
