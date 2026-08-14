---
title: 'HPX - The C++ Standard Library for Parallelism and Concurrency'
tags:
 - concurrency
 - task-based run time system
 - parallelism
 - distributed
authors:
 - name: Hartmut Kaiser
   orcid: 0000-0002-8712-2806
   affiliation: "1"
 - name: Patrick Diehl
   orcid: 0000-0003-3922-8419
   affiliation: "1"
 - name: Adrian S. Lemoine
   affiliation: "6"
 - name: Bryce Adelstein Lelbach
   orcid: 0000-0002-7995-5226
   affiliation: "5"
 - name: Parsa Amini
   orcid: 0000-0002-6439-8404
   affiliation: "1"
 - name: Agustín Berge
   affiliation: "6"
 - name: John Biddiscombe
   orcid: 0000-0002-6552-2833
   affiliation: "4"
 - name: Steven R. Brandt
   orcid: 0000-0002-7979-2906
   affiliation: "1"
 - name: Nikunj Gupta
   orcid: 0000-0003-0525-3667
   affiliation: "3"
 - name: Thomas Heller
   orcid: 0000-0003-2620-9438
   affiliation: "2"
 - name: Kevin Huck
   orcid: 0000-0001-7064-8417
   affiliation: "8"
 - name: Zahra Khatami
   orcid: 0000-0001-6654-6856
   affiliation: "7"
 - name: Alireza Kheirkhahan
   orcid: 0000-0002-4624-4647
   affiliation: "1"
 - name: Auriane Reverdell
   orcid: 0000-0002-5531-0458
   affiliation: "4"
 - name: Shahrzad Shirzad
   orcid: 0000-0001-9496-8044
   affiliation: "1"
 - name: Mikael Simberg
   orcid: 0000-0002-7238-8935
   affiliation: "4"
 - name: Bibek Wagle
   orcid: 0000-0001-6619-7115
   affiliation: "1"
 - name: Weile Wei
   orcid: 0000-0002-3065-4959
   affiliation: "1"
 - name: Tianyi Zhang
   orcid: 0000-0002-1000-4887
   affiliation: "6"
affiliations:
 - name: Center for Computation \& Technology, Louisiana State University, LA, Baton Rouge, United States of America
   index: 1
 - name: Exasol, Erlangen, Germany
   index: 2
 - name: Indian Institute of Technology, Roorkee, India
   index: 3
 - name: Swiss National Supercomputing Centre, Lugano, Switzerland
   index: 4
 - name: NVIDIA, CA, Santa Clara, United States of America
   index: 5
 - name: STE$||$AR Group
   index: 6
 - name: Oracle, CA, Redwood City, United States of America
   index: 7
 - name: Oregon Advanced Computing Institute for Science and Society (OACISS), University of Oregon, OR, Eugene, United States of America
   index: 8
date: 26.08.2020
bibliography: paper.bib
---

# Summary

The new challenges presented by exascale system architectures have resulted in
difficulty achieving the desired scalability using traditional distributed-memory
runtimes. Asynchronous many-task systems (AMT) are based on a new paradigm
showing promise in addressing these challenges, providing application
developers with a productive and performant approach to programming on next
generation systems.

HPX is a C++ Library for concurrency and parallelism that is
developed by The STE||AR Group, an international group of collaborators working
in the field of distributed and parallel programming
[@heller2017hpx;@hpx_github;@tabbal2011preliminary]. It is a runtime system
written using modern C++ techniques that are linked as part of an application.
HPX exposes extended services and functionalities supporting the implementation
of parallel, concurrent, and distributed capabilities for applications in any
domain; it has been used in scientific computing, gaming, finances, data
mining, and other fields.

The HPX AMT runtime system attempts to solve some problems the community
is facing when it comes to creating scalable parallel applications that expose
excellent parallel efficiency and a high resource utilization. First, it exposes
a C++ standards conforming API that unifies syntax and semantics for local and
remote operations. This significantly simplifies writing codes that strive to
effectively utilize different types of available parallelism in today's machines
in a coordinated way (i.e., on-node, off-node, and accelerator-based parallelism).
Second, HPX implements an asynchronous C++ standard programming model that has the
emergent property of semi-automatic parallelization of the user's code. The
provided API (especially when used in conjunction with the new C++20 `co_await`
keyword [@standard2020programming]) enables intrinsic overlap of computation and
communication, prefers moving work to data over moving data to work, and exposes
minimal overheads from its lightweight threading subsystem, ensuring efficient
fine-grained parallelization and minimal-overhead synchronization and context
switching. This programming model natively ensures high-system utilization and
perfect scalability.

A detailed comparison of HPX with various other AMTs is given in [@thoman2018taxonomy].
Some notable AMT solutions are: Uintah [@germain2000uintah], Chapel [@chamberlain2007parallel],
Charm++ [@kale1993charm], Kokkos [@edwards2014kokkos], Legion [@bauer2012legion],
and PaRSEC [@bosilca2013parsec]. Note that we only refer to distributed memory solutions,
since this is an important feature for scientific applications to run large scale simulations.
The major showpiece of HPX compared to the mentioned distributed AMTs is its future-proof C++
standards conforming API and the exposed asynchronous programming model.

HPX's main goal is to
improve efficiency and scalability of parallel applications by increasing
resource utilization and reducing synchronization overheads through providing an
asynchronous API and employing adaptive scheduling. The consequent use of
_Futures_ intrinsically enables overlap of computation and communication and
constraint-based synchronization. HPX is able to maintain a balanced load among
all the available resources resulting in significantly reducing processor
starvation and effective latencies while controlling overheads. HPX fully
conforms to the C++ ISO standards and implements the standardized concurrency
mechanisms and parallelism facilities. Further, HPX extends those facilities to
distributed use cases, thus enabling syntactic and semantic equivalence of local
and remote operations on the API level. HPX uses the concept of C++ _Futures_ to
transform sequential algorithms into wait-free asynchronous executions.
The use of _Futurization_ enables the automatic creation of dynamic data flow
execution trees of potentially millions of lightweight HPX tasks executed in the
proper order. HPX also provides a work-stealing task scheduler that takes care
of fine-grained parallelizations and automatic load balancing. Furthermore,
HPX implements functionalities proposed as part of the ongoing C++
standardization process.

![Sketch of HPX's architecture with all the components and their interactions.\label{fig:architecture}](hpx_architecture.pdf)


\autoref{fig:architecture} sketches HPX's architecture. The components of HPX
and their references are listed below:

**Threading Subsystem** [@kaiser2009parallex] The thread manager manages the
 light-weight user level threads created by HPX. These light-weight threads
 have extremely short context switching times, resulting in reduced latencies
 even for very short operations. This also ensures reduced synchronization
 overheads for coordinating execution between different threads. HPX provides
 a set of scheduling policies that enable the user to flexibly customize the
 execution of HPX threads. Work-stealing and work-sharing policies ensure
 automatic local load balancing of tasks, which is important for achieving high
 system utilization and good scalability of the user's code.

**Active Global Address Space (AGAS)** [@kaiser2014hpx;@amini2019agas]
 To support distributed objects, HPX supports a component for resolving
 global addresses that extends the Partitioned Global Address Space
 (PGAS) model, enabling dynamic runtime-based resource allocation and
 data placement.
 This layer enables HPX to expose a uniform API for local and remote
 execution. Unlike PGAS, AGAS provides the user with the ability to
 transparently move global objects in between nodes of a distributed computer
 system without changing the object's global address. This capability is
 fundamental for supporting load balancing via object migration.

**Parcel Transport Layer** [@kaiser2009parallex;@biddiscombe2017zero]
 This component is an active-message networking layer.
 The parcelport leverages AGAS in order to deliver messages to and to launch
 functions on global objects regardless of their current placement in a
 distributed system.
 Additionally, its asynchronous protocol enables the
 parcelport to implicitly overlap communication and computation.
 The parcelport is modular to support multiple communication library
 backends. By default, HPX supports TCP/IP, Message Passing Interface (MPI),
 and libfabric [@daiss2019piz].

**Performance counters** [@grubel2016dynamic]
 HPX provides its users with a uniform suite of globally accessible
 performance counters to monitor system metrics *in-situ*. These counters have
 their names registered with AGAS, which enables the users to
 easily query for different metrics at runtime.
 Additionally, HPX provides an API for users to create their
 own application-specific counters to gather information customized to their
 own application. These user-defined counters are exposed through the same
 interface as their predefined counterparts.
 By default, HPX provides performance counters for its own components, such as
 networking, AGAS operations, thread scheduling, and various statistics.

**Policy Engine/Policies** [@huck2015autonomic;@khatami2017hpx;@laberge2019scheduling]
 Often, modern applications must adapt to runtime environments
 to ensure acceptable performance. Autonomic Performance Environment for
 Exascale (APEX) enables this flexibility by measuring HPX tasks, monitoring
 system utilization, and accepting user provided policies
 that are triggered by defined events.
 In this way, features such as parcel coalescing [@wagle2018methodology] can
 adapt to the current phase of an application or even state of a system.

**Accelerator Support**
 HPX has support for several methods of integration with GPUs:
 HPXCL [@diehl2018integration; @martin_stumpf_2018_1409043] and HPX.Compute
 [@copik2017using].
 HPXCL provides users the ability to manage GPU kernels through a
 global object. This enables HPX to coordinate the launching and
 synchronization of CPU and GPU code.
 HPX.Compute [@copik2017using] aims to provide a single-source
 solution to heterogeneity by automatically generating GPU kernels
 from C++ code. This enables HPX to launch both CPU and GPU kernels
 as dictated by the current state of the system. Support for integrating
 HPX with Kokkos [@edwards2014kokkos] is currently being developed. This
 integration already has added HPX as an asynchronous backend to Kokkos and
 will expose Kokkos' accelerator functionalities through HPX's asynchronous APIs
 in a C++ standards-conforming way.

**Local Control Objects (synchronization support facilities)**
 HPX has support for many of the C++20 primitives, such as `hpx::latch`,
 `hpx::barrier`, and `hpx::counting_semaphore` to synchronize the execution of
 different threads allowing overlapping computation and communication. These
 facilities fully conform to the C++20 standard [@standard2020programming].
 For asynchronous computing HPX provides `hpx::async` and `hpx::future`; see
 the second example in the next section.

**Software Resilience**
 HPX supports software-level resilience [@gupta2020implementing] through its
 resiliency API, such as `hpx::async_replay` and `hpx::async_replicate` and
 its dataflow counterparts `hpx::dataflow_replay` and
 `hpx::dataflow_replicate`. These APIs are resilient against memory bit
 flips and other hardware errors.
 HPX provides an easy method to port codes to the resilient API by replacing
 `hpx::async` or `hpx::dataflow` with its resilient API counterparts everywhere
 in the code without making any other changes.

**C++ Standards conforming API**
 HPX implements all the C++17 parallel algorithms [@standard2017programming]
 and extends those with asynchronous versions. Here, HPX provides the
 `hpx::execution::seq` and `hpx::execution::par` execution policies, and (as an
 extension) their asynchronous equivalents
 `hpx::execution::seq(hpx::execution::task)` and
 `hpx::execution::par(hpx::execution::task)` (see the first code example
 below). HPX also implements the C++20
 concurrency facilities and APIs [@standard2020programming], such as
 `hpx::jthread`, `hpx::latch`, `hpx::barrier`, etc.

# Applications

HPX is utilized in a diverse set of applications: 

- Scientific computing
   * [Octo-Tiger](https://github.com/STEllAR-GROUP/octotiger)
   [@daiss2019piz;@heller2019harnessing;@pfander2018accelerating], an
   astrophysics code for stellar mergers.
   * [libGeoDecomp](https://github.com/gentryx/libgeodecomp)
   [@Schafer:2008:LGL:1431669.1431721], an auto-parallelizing library to speed
   up stencil-code-based computer simulations.
   * [NLMech](https://github.com/nonlocalmodels) [@diehl2018implementation], a
   simulation tool for non-local models, e.g. Peridynamics.
   * [Dynamical Cluster Approximation](https://github.com/CompFUSE/DCA) (DCA++)
   [@hahner2020dca], a high-performance research software framework to solve 
   quantum many-body problems with cutting edge quantum cluster algorithms. 

- Libraries
   * [hpxMP](https://github.com/STEllAR-GROUP/hpxMP)
   [@zhang2019introduction; @zhang2020supporting] a modern OpenMP implementation
   leveraging HPX that supports shared memory multithread programming. 
   * [Kokkos](https://github.com/kokkos/kokkos) [@10.1016/j.jpdc.2014.07.003],
   the C++ Performance Portability Programming EcoSystem. 
   * [Phylanx](https://github.com/STEllAR-GROUP/phylanx)
   [@tohid2018asynchronous;@wagle2019runtime] An Asynchronous Distributed C++
   Array Processing Toolkit.

For a updated list of applications, we refer to the
corresponding [HPX website](https://hpx.stellar-group.org/hpx-users/).

# Example code

The following is an example of HPX's parallel algorithms API using execution
policies as defined in
the C++17 standard [@standard2017programming]. HPX implements all the
parallel algorithms defined therein. The parallel algorithms extend the classic
STL algorithms by adding a first argument (called execution policy).
The `hpx::execution::seq` implies sequential execution while `hpx::execution::par`
will execute the algorithm in parallel.
HPX's parallel algorithm library API is completely standards conforming.

```cpp
#include <hpx/hpx.hpp>
#include <iostream>
#include <vector>

int main()
{
 std::vector<int> values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

 // Compute the sum in a sequential fashion
 int sum1 = hpx::reduce(
 hpx::execution::seq, values.begin(), values.end(), 0);
 std::cout << sum1 << '\n'; // will print 55

 // Compute the sum in a parallel fashion based on a range of values
 int sum2 = hpx::ranges::reduce(hpx::execution::par, values, 0);
 std::cout << sum2 << '\n'; // will print 55 as well

 return 0;
}
```

Example for the HPX's concurrency API where the Taylor series for the $\sin(x)$
function is computed. The Taylor series is given by,

$$ \sin(x) \approx = \sum\limits_{n=0}^N (-1)^{n-1} \frac{x^{2n}}{(2n)!}.$$

For the concurrent computation, the interval $[0, N]$ is split in two
partitions from $[0, N/2]$ and $[(N/2)+1, N]$, and these are computed
asynchronously using `hpx::async`. Note that each asynchronous function call
returns an `hpx::future` which is needed to synchronize the collection
of the partial results. The future has a `get()` method that returns the result
once the computation of the Taylor function finished. If the result is not ready
yet, the current thread is suspended until the result is ready. Only if
`f1` and `f2` are ready, the overall result will be printed to the standard
output stream.

```cpp
#include <hpx/hpx.hpp>
#include <cmath>
#include <iostream>

// Define the partial taylor function
double taylor(size_t begin, size_t end, size_t n, double x)
{
 double denom = factorial(2 * n);
 double res = 0;
 for (size_t i = begin; i != end; ++i)
 {
 res += std::pow(-1, i - 1) * std::pow(x, 2 * n) / denom;
 }
 return res;
}

int main()
{
 // Compute the Talor series sin(2.0) for 100 iterations
 size_t n = 100;

 // Launch two concurrent computations of each partial result
 hpx::future<double> f1 = hpx::async(taylor, 0, n / 2, n, 2.);
 hpx::future<double> f2 = hpx::async(taylor, (n / 2) + 1, n, n, 2.);

 // Introduce a barrier to gather the results
 double res = f1.get() + f2.get();

 // Print the result
 std::cout << "Sin(2.) = " << res << std::endl;
}
```

Please report any bugs or feature requests on the
[HPX GitHub page](https://github.com/TheHPXProject/hpx).

# Acknowledgments

We would like to acknowledge the National Science Foundation (NSF), the U.S.
Department of Energy (DoE), the Defense Technical Information Center (DTIC), the
Defense Advanced Research Projects Agency (DARPA), the Center for Computation
and Technology (CCT) at Louisiana State University (LSU), the Swiss National
Supercomputing Centre (CSCS), the Department of Computer Science 3 - Computer
Architecture at the University of Erlangen Nuremberg who fund and support our
work, and the Heterogeneous System Architecture (HSA) Foundation.

We would also like to thank the following organizations for granting us
allocations of their compute resources: LSU HPC, Louisiana Optical Network
Iniative (LONI), the Extreme Science and Engineering Discovery Environment
(XSEDE), the National Energy Research Scientific Computing Center (NERSC), the
Oak Ridge Leadership Computing Facility (OLCF), Swiss National Supercomputing
Centre (CSCS/ETHZ), the Juelich Supercomputing Centre (JSC), and the Gauss
Center for Supercomputing.

At the time the paper was written, HPX was directly funded by the following
grants:

- The National Science Foundation through awards 1339782 (STORM) and 1737785
 (Phylanx).

- The Department of Energy (DoE) through the awards DE-AC52-06NA25396 (FLeCSI)
 DE-NA0003525 (Resilience), and DE-AC05-00OR22725 (DCA++).

- The Defense Technical Information Center (DTIC) under contract
 FA8075-14-D-0002/0007.

- The Bavarian Research Foundation (Bayerische Forschungsstiftung) through the
 grant AZ-987-11.

- The European Commission's Horizon 2020 programme through the grant
 H2020-EU.1.2.2. 671603 (AllScale).


For a updated list of previous and current funding, we refer to the
corresponding [HPX website](http://hpx.stellar-group.org/funding-acknowledgements/).

# References
