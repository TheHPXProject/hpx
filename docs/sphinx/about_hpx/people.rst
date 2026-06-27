..
    Copyright (C) 2007-2022 Hartmut Kaiser
    Copyright (C) 2016-2018 Adrian Serio

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _people:

======
People
======

The |stellar|_ Group (pronounced as stellar) stands for "\ **S**\ ystems \
**T**\ echnology, \ **E**\ mergent Para\ **ll**\ elism, and \ **A**\ lgorithm \
**R**\ esearch". We are an international group of faculty, researchers, and
students working at various institutions around the world. The goal of the
|stellar|_ Group is to promote the development of scalable parallel applications
by providing a community for ideas, a framework for collaboration, and a
platform for communicating these concepts to the broader community.

Our work is focused on building technologies for scalable parallel applications.
|hpx|, our general purpose C++ runtime system for parallel and distributed
applications, is no exception. We use |hpx| for a broad range of scientific
applications, helping scientists and developers to write code which scales
better and shows better performance compared to more conventional programming
models such as MPI.

|hpx| is based on *ParalleX* which is a new (and still experimental) parallel
execution model aiming to overcome the limitations imposed by the current
hardware and the techniques we use to write applications today. Our group
focuses on two types of applications - those requiring excellent strong scaling,
allowing for a dramatic reduction of execution time for fixed workloads and
those needing highest level of sustained performance through massive
parallelism. These applications are presently unable (through conventional
practices) to effectively exploit a relatively small number of cores in a
multi-core system. By extension, these application will not be able to exploit
high-end exascale computing systems which are likely to employ hundreds of
millions of such cores by the end of this decade.

Critical bottlenecks to the effective use of new generation high performance
computing (HPC) systems include:

* *Starvation*: due to lack of usable application parallelism and means of
  managing it,
* *Overhead*: reduction to permit strong scalability, improve efficiency, and
  enable dynamic resource management,
* *Latency*: from remote access across system or to local memories,
* *Contention*: due to multicore chip I/O pins, memory banks, and system
  interconnects.

The ParalleX model has been devised to address these challenges by enabling a
new computing dynamic through the application of message-driven computation in a
global address space context with lightweight synchronization. The work on |hpx|
is centered around implementing the concepts as defined by the ParalleX model.
|hpx| is currently targeted at conventional machines, such as classical Linux
based Beowulf clusters and SMP nodes.

We fully understand that the success of |hpx| (and ParalleX) is very much the
result of the work of many people. To see a list of who is contributing see our
tables below.

|hpx| contributors
==================

.. table:: Contributors

   ======================= ================ =====
   Name                    Institution      Email
   ======================= ================ =====
   Hartmut Kaiser          |cct|_, |lsu|_   |email_hkaiser|
   Thomas Heller           |inf3|_, |fau|_  |email_theller|
   Agustin Berge                            |email_aberge|
   Mikael Simberg          |cscs|_          |email_msimberg|
   John Biddiscombe        |cscs|_          |email_jbiddiscombe|
   Anton Bikineev          |cct|_, |lsu|_   |email_abikineev|
   Martin Stumpf           |inf3|_, |fau|_  |email_mstumpf|
   Bryce Adelstein Lelbach                  |email_blelbach|
   Shuangyang Yang         |cct|_, |lsu|_   |email_syang|
   Jeroen Habraken                          |email_jhabraken|
   Steven Brandt           |cct|_, |lsu|_   |email_sbrandt|
   Antoine Tran Tan        |psu|_,          |email_atrantan|
   Adrian S. Lemoine       |amd|_           |email_alemoine|
   Maciej Brodowicz                         |email_mbrodowicz|
   Giannis Gonidelis       |cct|_, |lsu|_   |email_ggonidelis|
   ======================= ================ =====

Contributors to this document
=============================

.. table:: Documentation authors

  ======================= ===================== =====
  Name                    Institution           Email
  ======================= ===================== =====
  Hartmut Kaiser          |cct|_, |lsu|_        |email_hkaiser|
  Thomas Heller           |inf3|_, |fau|_       |email_theller|
  Bryce Adelstein Lelbach                       |email_blelbach|
  Vinay C Amatya          |cct|_, |lsu|_        |email_vamatya|
  Steven Brandt           |cct|_, |lsu|_        |email_sbrandt|
  Maciej Brodowicz                              |email_mbrodowicz|
  Adrian S. Lemoine       |amd|_                |email_alemoine|
  Rebecca Stobaugh                              |email_rstobaugh|
  Dimitra Karatza         |tud_eemcs|_, |tud|_  |email_dkaratza|
  Bhumit Attarde                                |email_bhumitattarde|
  ======================= ===================== =====

Acknowledgements
================

Thanks also to the following people who contributed directly or indirectly to
the project through discussions, pull requests, documentation patches, etc.

* Panos Syskakis for benchmarking and optimizing our parallel algorithms.
* Shreyas Atre, for contributing fixes to our implementation of
  senders/receivers and extending our coroutines integration with senders/receivers.
* Alexander Neumann, for contributing fixes to the cmake build system.
* Dimitra Karatza, for her work on refactoring the documentation and providing
  a new user-friendly environment during and after Google Season of Docs 2021.
* Srinivas Yadav, for his work on SIMD support in algorithms before and during
  Google Summer of Code 2021.
* Akhil Nair, for his work on adapting algorithms to C++20 before and during
  Google Summer of Code 2021.
* Alexander Toktarev, for updating the parallel algorithm customization points
  to use ``tag_fallback_invoke`` for the default implementations.
* Brice Goglin, for reporting and helping fix issues related to the integration
  of hwloc in |hpx|.
* Giannis Gonidelis, for his work on the ranges adaptation during the
  Google Summer of Code 2020.
* Auriane Reverdell (|cscs|_), for her tireless work on refactoring our CMake
  setup and modularizing |hpx|.
* Christopher Hinz, for his work on refactoring our CMake setup.
* Weile Wei, for fixing |hpx| builds with CUDA on Summit.
* Severin Strobl, for fixing our CMake setup related to linking and adding new
  entry points to the |hpx| runtime.
* Rebecca Stobaugh, for her major documentation review and contributions
  during and after the 2019 Google Season of Documentation.
* Jan Melech, for adding automatic serialization of simple structs.
* Austin McCartney, for adding concept emulation of the Ranges TS bidirectional
  and random access iterator concepts.
* Marco Diers, reporting and fixing issues related PMIx.
* Maximilian Bremer, for reporting multiple issues and extending the component
  migration tests.
* Piotr Mikolajczyk, for his improvements and fixes to the set and count
  algorithms.
* Grant Rostig, for reporting several deficiencies on our web pages.
* Jakub Golinowski, for implementing an |hpx| backend for OpenCV and in the
  process improving documentation and reporting issues.
* Mikael Simberg (|cscs|_), for his tireless help cleaning up and maintaining
  |hpx|.
* Tianyi Zhang, for his work on HPXMP.
* Shahrzad Shirzad, for her contributions related to Phylanx.
* Christopher Ogle, for his contributions to the parallel algorithms.
* Surya Priy, for his work with statistic performance counters.
* Anushi Maheshwari, for her work on random number generation.
* Bruno Pitrus, for his work with parallel algorithms.
* Nikunj Gupta, for rewriting the implementation of ``hpx_main.hpp`` and for his
  fixes for tests.
* Christopher Taylor, for his interest in |hpx| and the fixes he provided. Chris
  also contributed support for RISC-V architectures.
* Shoshana Jakobovits, for her work on the resource partitioner.
* Denis Blank, who re-wrote our unwrapped function to accept plain values
  arbitrary containers, and properly deal with nested futures.
* Ajai V. George, who implemented several of the parallel algorithms.
* Taeguk Kwon, who worked on implementing parallel algorithms as well as
  adapting the parallel algorithms to the Ranges TS.
* Zach Byerly (|lsu|_), who in his work developing applications on top of |hpx|
  opened tickets and contributed to the |hpx| examples.
* Daniel Estermann, for his work porting |hpx| to the Raspberry Pi.
* Alireza Kheirkhahan (|lsu|_), who built and administered our local cluster as
  well as his work in distributed IO.
* Abhimanyu Rawat, who worked on stack overflow detection.
* David Pfander, who improved signal handling in |hpx|, provided his
  optimization expertise, and worked on incorporating the Vc vectorization into
  |hpx|.
* Denis Demidov, who contributed his insights with VexCL.
* Khalid Hasanov, who contributed changes which allowed to run |hpx| on 64Bit
  power-pc architectures.
* Zahra Khatami (|lsu|_), who contributed the prefetching iterators and the
  persistent auto chunking executor parameters implementation.
* Marcin Copik, who worked on implementing GPU support for C++AMP and HCC. He
  also worked on implementing a HCC backend for |hpx_compute|.
* Minh-Khanh Do, who contributed the implementation of several segmented
  algorithms.
* Bibek Wagle (|lsu|_), who worked on fixing and analyzing the performance of
  the :term:`parcel` coalescing plugin in |hpx|.
* Lukas Troska, who reported several problems and contributed various test cases
  allowing to reproduce the corresponding issues.
* Andreas Schaefer, who worked on integrating his library (|lgd|_) with |hpx|.
  He reported various problems and submitted several patches to fix issues
  allowing for a better integration with |lgd|_.
* Satyaki Upadhyay, who contributed several examples to |hpx|.
* Brandon Cordes, who contributed several improvements to the inspect tool.
* Harris Brakmic, who contributed an extensive build system description for
  building |hpx| with Visual Studio.
* Parsa Amini (|lsu|_), who refactored and simplified the implementation of
  :term:`AGAS` in |hpx| and who works on its implementation and optimization.
* Luis Martinez de Bartolome who implemented a build system extension for |hpx|
  integrating it with the |conan|_ C/C++ package manager.
* Vinay C Amatya (|lsu|_), who contributed to the documentation and provided
  some of the |hpx| examples.
* Kevin Huck and Nick Chaimov (|ou|_), who contributed the integration of APEX
  (Autonomic Performance Environment for eXascale) with |hpx|.
* Francisco Jose Tapia, who helped with implementing the parallel sort algorithm
  for |hpx|.
* Patrick Diehl, who worked on implementing CUDA support for our companion
  library targeting GPGPUs (|hpxcl|_).
* Eric Lemanissier contributed fixes to allow compilation using the MingW
  toolchain.
* Nidhi Makhijani who helped cleaning up some enum consistencies in |hpx| and
  contributed to the resource manager used in the thread scheduling subsystem.
  She also worked on |hpx| in the context of the Google Summer of Code 2015.
* Larry Xiao, Devang Bacharwar, Marcin Copik, and Konstantin Kronfeldner who
  worked on |hpx| in the context of the Google Summer of Code program 2015.
* Daniel Bourgeois (|cct|_) who contributed to |hpx| the implementation of
  several parallel algorithms (as proposed by |cpp11_n4107|_).
* Anuj Sharma and Christopher Bross (|inf3|_), who worked on |hpx| in the
  context of the |gsoc|_ program 2014.
* Martin Stumpf (|inf3|_), who rebuilt our contiguous testing infrastructure
  (see the |hpx_buildbot|_). Martin is also working on |hpxcl|_ (mainly all work
  related to |opencl|_) and implementing an |hpx| backend for |pocl|_, a
  portable computing language solution based on |opencl|_.
* Grant Mercer (|unlv|_), who helped creating many of the parallel algorithms
  (as proposed by |cpp11_n4107|_).
* Damond Howard (|lsu|_), who works on |hpxcl|_ (mainly all work related to
  |cuda|_).
* Christoph Junghans (Los Alamos National Lab), who helped making our
  buildsystem more portable.
* Antoine Tran Tan (Laboratoire de Recherche en Informatique, Paris), who worked
  on integrating |hpx| as a backend for |nt2|_. He also contributed an
  implementation of an API similar to Fortran co-arrays on top of |hpx|.
* John Biddiscombe (|cscs|_), who helped with the BlueGene/Q port of |hpx|,
  implemented the parallel sort algorithm, and made several other contributions.
* Erik Schnetter (Perimeter Institute for Theoretical Physics), who greatly
  helped to make |hpx| more robust by submitting a large amount of problem
  reports, feature requests, and made several direct contributions.
* Mathias Gaunard (Metascale), who contributed several patches to reduce compile
  time warnings generated while compiling |hpx|.
* Andreas Buhr, who helped with improving our documentation, especially by
  suggesting some fixes for inconsistencies.
* Patricia Grubel (|nmsu|_), who contributed the description of the different
  |hpx| thread scheduler policies and is working on the performance analysis of
  our thread scheduling subsystem.
* Lars Viklund, whose wit, passion for testing, and love of odd architectures
  has been an amazing contribution to our team. He has also contributed platform
  specific patches for FreeBSD and MSVC12.
* Agustin Berge, who contributed patches fixing some very nasty hidden template
  meta-programming issues. He rewrote large parts of the API elements ensuring
  strict conformance with the C++ ISO Standard.
* Anton Bikineev for contributing changes to make using ``boost::lexical_cast``
  safer, he also contributed a thread safety fix to the iostreams module. He
  also contributed a complete rewrite of the serialization infrastructure
  replacing Boost.Serialization inside |hpx|.
* Pyry Jahkola, who contributed the Mac OS build system and build documentation
  on how to build |hpx| using Clang and libc++.
* Mario Mulansky, who created an |hpx| backend for his Boost.Odeint library, and
  who submitted several test cases allowing us to reproduce and fix problems in
  |hpx|.
* Rekha Raj, who contributed changes to the description of the Windows build
  instructions.
* Jeremy Kemp how worked on an |hpx| OpenMP backend and added regression tests.
* Alex Nagelberg for his work on implementing a C wrapper API for |hpx|.
* Chen Guo, helvihartmann, Nicholas Pezolano, and John West who added and
  improved examples in |hpx|.
* Joseph Kleinhenz, Markus Elfring, Kirill Kropivyansky, Alexander Neundorf,
  Bryant Lam, and Alex Hirsch who improved our CMake.
* Tapasweni Pathak, Praveen Velliengiri, Jean-Loup Tastet, Michael Levine, Aalekh Nigam,
  HadrienG2, Prayag Verma, lslada, Alex Myczko, and Avyav Kumar
  who improved the documentation.
* Jayesh Badwaik, J. F. Bastien, Christoph Garth, Christopher Hinz, Brandon
  Kohn, Mario Lang, Maikel Nadolski, pierrele, hendrx, Dekken, woodmeister123,
  xaguilar, Andrew Kemp, Dylan Stark, Matthew Anderson, Jeremy Wilke, Jiazheng
  Yuan, CyberDrudge, david8dixon, Maxwell Reeser, Raffaele Solca, Marco
  Ippolito, Jules Penuchot, Weile Wei, Severin Strobl, Kor de Jong, albestro,
  Jeff Trull, Yuri Victorovich, and Gregor Daiss who contributed to the general
  improvement of |hpx|.

|stellar_hpx_funding|_ lists current and past funding sources for |hpx|. Special thanks to
|gsoc|_ and |gsod|_ for the continuous support they provide which helps us enhance both our
code and our documentation.
