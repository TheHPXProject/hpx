#!/bin/bash
#
# Copyright (c) 2009-2012 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)

usage()
{
    echo "Usage: $0 -d directory -v version [args]"
    echo
    echo "This script downloads and builds the Boost C++ Libraries."
    echo
    echo "Options:"
    echo "  -d    Directory where Boost should be built."
    echo "  -v    Version of Boost to build (format: X.YY.Z)"
    echo "  -n    Don't download Boost (expects tarball named boost_X.YY.Z.tar.bz2 in the -d directory) [default: 1.55.0]"
    echo "  -x    Libraries to exclude (format: exclude0,exclude1...) [default: mpi,graph_parallel,python]"
    echo "  -t    Number of threads to use while building [default: number of processors]"
    echo "  -c    Compiler [default: automatically detected]"
}

DIRECTORY=

# Dot version, e.g. X.YY.Z
DOT_VERSION=1.55.0

# Underscore version, e.g. X_YY_Z
US_VERSION=1_55_0

DOWNLOAD=1

# HPX does not need these, and they have external dependencies, so skip them.
EXCLUDES=mpi,graph_parallel,python

# Physical, not logical cores.
THREADS=`grep -c ^processor /proc/cpuinfo`

COMPILER=

###############################################################################
# Argument parsing
while getopts "hnt:d:v:c:x:" OPTION; do case $OPTION in
    h)
        usage
        exit 0
        ;;
    n)
        DOWNLOAD=0
        ;;
    d)
        # Try to make the directories.
        mkdir -p "$OPTARG/release" > /dev/null 2>&1
        mkdir -p "$OPTARG/debug" > /dev/null 2>&1

        if [[ -d "$OPTARG/release" && -w $OPTARG/release ]] && \
           [[ -d "$OPTARG/debug" && -w $OPTARG/debug   ]];
        then
            DIRECTORY=$OPTARG
        else
            echo "ERROR: -d argument was invalid"; echo
            usage
            exit 1
        fi
        ;;
    v)
        if [[ $OPTARG =~ ^[0-9][.][0-9][0-9][.][0-9]$ ]]; then
            DOT_VERSION=$OPTARG
            US_VERSION=${OPTARG//./_}
        else
            echo "ERROR: -v argument was invalid"; echo
            usage
            exit 1
        fi
        ;;
    x)
        EXCLUDES=$OPTARG
        ;;
    t)
        if [[ $OPTARG =~ ^[0-9]+$ ]]; then
            THREADS=$OPTARG
        else
            echo "ERROR: -t argument was invalid"; echo
            usage
            exit 1
        fi
        ;;
    c)
        COMPILER=$OPTARG
        ;;
    ?)
        usage
        exit 1
        ;;
esac; done

if ! [[ $DIRECTORY ]]; then
    echo "ERROR: no directory specified"; echo
    usage
    exit 1
fi

if ! [[ $DOT_VERSION && $US_VERSION ]]; then
    echo "ERROR: no version specified"; echo
    usage
    exit 1
fi

if [[ $EXCLUDES ]]; then
    EXCLUDES="--without-libraries=$EXCLUDES"
fi

if [[ $COMPILER ]]; then
    COMPILER="--with-toolset=$COMPILER"
fi

###############################################################################
DIRECTORY=$(cd $DIRECTORY; pwd)
ORIGINAL_DIRECTORY=$PWD

BJAM=$DIRECTORY/source/b2

error()
{
    cd $ORIGINAL_DIRECTORY
    exit 1
}

cd "$DIRECTORY"

if [[ $DOWNLOAD == "1" ]]; then
    wget downloads.sourceforge.net/sourceforge/boost/boost/$DOT_VERSION/boost_$US_VERSION.tar.bz2
    if ! [[ $? == "0" ]]; then echo "ERROR: Unable to download Boost"; error; fi
fi

tar --no-same-owner -xf boost_$US_VERSION.tar.bz2
if ! [[ $? == "0" ]]; then echo "ERROR: Unable to unpack `pwd`/boost_$US_VERSION.tar.bz2"; error; fi

mv boost_$US_VERSION source

cd "$DIRECTORY/source"

# Bootstrap the Boost build system, Boost.Build.
$DIRECTORY/source/bootstrap.sh $EXCLUDES $COMPILER

$BJAM --stagedir="$DIRECTORY/debug/stage" variant=debug -j${THREADS}
if ! [[ $? == "0" ]]; then echo "ERROR: Debug build of Boost failed"; error; fi

$BJAM --stagedir="$DIRECTORY/release/stage" variant=release -j${THREADS}
if ! [[ $? == "0" ]]; then echo "ERROR: Release build of Boost failed"; error; fi

# Build the Boost.Wave preprocessor.
cd "$DIRECTORY/source/tools/wave/build"
$BJAM dist-bin -j${THREADS} variant=release

# Build the Quickbook documentation framework.
cd "$DIRECTORY/source/tools/quickbook"
$BJAM dist-bin -j${THREADS} variant=release

# Copy over the BoostBook DTD and XML code to the staging directory.
cd "$DIRECTORY/source/tools"
$BJAM dist-share-boostbook

# Build the auto_index indexing tool.
cd "$DIRECTORY/source/tools/auto_index/build"
$BJAM i -j${THREADS} variant=release

# These links are necessary to ensure that the stage directories are usable
# Boost source trees.
create_links()
{
    ln -fs "$DIRECTORY/source/bjam" bjam
    ln -fs "$DIRECTORY/source/boost" boost
    ln -fs "$DIRECTORY/source/boost-build.jam" boost-build.jam
    ln -fs "$DIRECTORY/source/boostcpp.jam" boostcpp.jam
    ln -fs "$DIRECTORY/source/boost.css" boost.css
    ln -fs "$DIRECTORY/source/boost.png" boost.png
    ln -fs "$DIRECTORY/source/dist" dist
    ln -fs "$DIRECTORY/source/doc" doc
    ln -fs "$DIRECTORY/source/index.htm" index.htm
    ln -fs "$DIRECTORY/source/index.html" index.html
    ln -fs "$DIRECTORY/source/Jamroot" Jamroot
    ln -fs "$DIRECTORY/source/libs" libs
    ln -fs "$DIRECTORY/source/LICENSE_1_0.txt" LICENSE_1_0.txt
    ln -fs "$DIRECTORY/source/project-config.jam" project-config.jam
    ln -fs "$DIRECTORY/source/rst.css" rst.css
    ln -fs "$DIRECTORY/source/tools" tools
}

cd "$DIRECTORY/debug"
create_links

cd "$DIRECTORY/release"
create_links

echo
echo "Successfully built Boost ${DOT_VERSION}"
echo
echo "Debug root:"
echo "  Boost_ROOT=$DIRECTORY/debug"
echo
echo "Release root:"
echo "  Boost_ROOT=$DIRECTORY/release"



