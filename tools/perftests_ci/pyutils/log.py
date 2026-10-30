# -*- coding: utf-8 -*-
'''
Copyright (c) 2020 ETH Zurich

SPDX-License-Identifier: BSL-1.0
Distributed under the Boost Software License, Version 1.0. (See accompanying
file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''

import contextlib
import logging
import sys
import textwrap


_logger = logging.getLogger('pyutils')
_logger.setLevel(logging.DEBUG)

# No colon after the time: CTest takes "<name>:<number>: <text>" for a
# compiler error, so "12:34:56: text" would fail the build steps that run the
# performance tests
_formatter = logging.Formatter('%(levelname)s %(asctime)s - %(message)s',
                               '%Y-%m-%d %H:%M:%S')


_streamhandler = logging.StreamHandler()
_streamhandler.setFormatter(_formatter)
_streamhandler.setLevel(logging.WARNING)
_logger.addHandler(_streamhandler)


def log_to_file(logfile):
    filehandler = logging.FileHandler(logfile)
    filehandler.setFormatter(_formatter)
    filehandler.setLevel(logging.DEBUG)
    _logger.addHandler(filehandler)
    info('Logging to file', logfile)


def set_verbosity(level):
    if level <= 0:
        _streamhandler.setLevel(logging.WARNING)
    elif level == 1:
        _streamhandler.setLevel(logging.INFO)
    else:
        _streamhandler.setLevel(logging.DEBUG)


@contextlib.contextmanager
def exception_logging():
    try:
        yield
    except Exception:
        _logger.exception(f'{"Fatal error: exception was raised"}')
        sys.exit(1)


def _format_message(message, details):
    message = str(message)
    if details is None:
        return message
    details = str(details)
    if details.count('\n') == 0:
        if details.strip() == '':
            details = '[EMPTY]'
        return message + ': ' + details
    else:
        return message + ':\n' + textwrap.indent(details, '    ')


def debug(message, details=None):
    _logger.debug(_format_message(message, details))


def info(message, details=None):
    _logger.info(_format_message(message, details))


def warning(message, details=None):
    _logger.warning(_format_message(message, details))


def error(message, details=None):
    _logger.error(_format_message(message, details))
