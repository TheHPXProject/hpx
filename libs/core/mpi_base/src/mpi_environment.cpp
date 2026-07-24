//  Copyright (c) 2013-2015 Thomas Heller
//  Copyright (c)      2020 Google
//  Copyright (c)      2022 Patrick Diehl
//  Copyright (c)      2023 Hartmut Kaiser
//  Copyright (c)      2024 Jiakun Yan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/mpi_base.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/string_util.hpp>
#include <hpx/modules/util.hpp>

#include <cstddef>
#include <cstdlib>
#include <map>
#include <stdexcept>
#include <string>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

    int mpi_environment::MPI_MAX_TAG = 8192;

    namespace detail {

        bool detect_mpi_environment(
            util::runtime_configuration const& cfg, char const* default_env)
        {
#if defined(__bgq__)
            // If running on BG/Q, we can safely assume to always run in an
            // MPI environment
            return true;
#else
            std::string const mpi_environment_strings =
                cfg.get_entry("hpx.parcel.mpi.env", default_env);

            hpx::string_util::char_separator sep(";,: ");
            hpx::string_util::tokenizer const tokens(
                mpi_environment_strings, sep);
            for (auto const& tok : tokens)
            {
                if (char const* env = std::getenv(tok.c_str()))
                {
                    LBT_(debug)
                        << "Found MPI environment variable: " << tok << "="
                        << std::string(env) << ", enabling MPI support\n";
                    return true;
                }
            }

            LBT_(info) << "No known MPI environment variable found, disabling "
                          "MPI support\n";
            return false;
#endif
        }
    }    // namespace detail

    bool mpi_environment::check_mpi_environment(
        util::runtime_configuration const& cfg)
    {
#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_MPI)
        // We disable the MPI parcelport if any of these hold:
        //
        // - The parcelport is explicitly disabled
        // - The application is not run in an MPI environment
        // - The TCP parcelport is enabled and has higher priority
        if (get_entry_as(cfg, "hpx.parcel.mpi.enable", 1) == 0 ||
            (get_entry_as(cfg, "hpx.parcel.tcp.enable", 1) &&
                (get_entry_as(cfg, "hpx.parcel.tcp.priority", 1) >
                    get_entry_as(cfg, "hpx.parcel.mpi.priority", 0))) ||
            (get_entry_as(cfg, "hpx.parcel.lci.enable", 1) &&
                (get_entry_as(cfg, "hpx.parcel.lci.priority", 1) >
                    get_entry_as(cfg, "hpx.parcel.mpi.priority", 0))) ||
            (get_entry_as(cfg, "hpx.parcel.lcw.enable", 1) &&
                (get_entry_as(cfg, "hpx.parcel.lcw.priority", 1) >
                    get_entry_as(cfg, "hpx.parcel.mpi.priority", 0))))
        {
            LBT_(info) << "MPI support disabled via configuration settings\n";
            return false;
        }

        if (!detail::detect_mpi_environment(cfg, HPX_HAVE_PARCELPORT_MPI_ENV))
        {
            // log message was already generated
            return false;
        }

        return true;
#elif defined(HPX_HAVE_MODULE_MPI_BASE)
        // if MPI futures are enabled while networking is off we need to
        // check whether we were run using mpirun
        return detail::detect_mpi_environment(cfg, HPX_HAVE_PARCELPORT_MPI_ENV);
#else
        return false;
#endif
    }
}    // namespace hpx::util

#if (defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_MPI)) ||      \
    defined(HPX_HAVE_MODULE_MPI_BASE)

namespace hpx::util {

    mpi_environment::mutex_type mpi_environment::mtx_;
    bool mpi_environment::enabled_ = false;
    bool mpi_environment::has_called_init_ = false;
    int mpi_environment::provided_threading_flag_ = MPI_THREAD_SINGLE;
    MPI_Comm mpi_environment::communicator_ = MPI_COMM_NULL;

    int mpi_environment::is_initialized_ = -1;

    ///////////////////////////////////////////////////////////////////////////
    namespace {

        [[noreturn]] void throw_wrong_mpi_mode(int required, int provided)
        {
            std::map<int, char const*> levels = {
                {MPI_THREAD_SINGLE, "MPI_THREAD_SINGLE"},
                {MPI_THREAD_FUNNELED, "MPI_THREAD_FUNNELED"},
                {MPI_THREAD_SERIALIZED, "MPI_THREAD_SERIALIZED"},
                {MPI_THREAD_MULTIPLE, "MPI_THREAD_MULTIPLE"}};

            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::util::mpi_environment::init",
                "MPI doesn't implement minimal requested thread level, "
                "required "
                "{}, provided {}",
                levels[required], levels[provided]);
        }
    }    // namespace

    int mpi_environment::init(
        int*, char***, int const minimal, int const required, int& provided)
    {
        has_called_init_ = false;

        // Check if MPI_Init has been called previously
        int is_initialized = 0;
        int retval = MPI_Initialized(&is_initialized);
        if (MPI_SUCCESS != retval)
        {
            return retval;
        }

        if (!is_initialized)
        {
            retval = MPI_Init_thread(nullptr, nullptr, required, &provided);
            if (MPI_SUCCESS != retval)
            {
                return retval;
            }

            if (provided < minimal)
            {
                throw_wrong_mpi_mode(required, provided);
            }
            has_called_init_ = true;
        }
        else
        {
            // ask what MPI threading mode is active
            retval = MPI_Query_thread(&provided);
            if (MPI_SUCCESS != retval)
            {
                return retval;
            }

            if (provided < minimal)
            {
                throw_wrong_mpi_mode(required, provided);
            }
        }
        return retval;
    }

    ///////////////////////////////////////////////////////////////////////////
    void mpi_environment::init(
        int* argc, char*** argv, util::runtime_configuration& rtcfg)
    {
        if (enabled_)
            return;    // don't call twice

        int this_rank = -1;
        has_called_init_ = false;

        // We assume to use the MPI parcelport if it is not explicitly disabled
        enabled_ = check_mpi_environment(rtcfg);
        if (!enabled_)
        {
            rtcfg.add_entry("hpx.parcel.mpi.enable", "0");
            return;
        }

        rtcfg.add_entry("hpx.parcel.bootstrap", "mpi");

        int required = MPI_THREAD_SERIALIZED;
#if defined(HPX_HAVE_PARCELPORT_MPI_MULTITHREADED)
        required =
            (get_entry_as(rtcfg, "hpx.parcel.mpi.multithreaded", 1) != 0) ?
            MPI_THREAD_MULTIPLE :
            MPI_THREAD_SERIALIZED;

#if defined(MVAPICH2_VERSION) && defined(_POSIX_SOURCE)
        // This enables multi threading support in MVAPICH2 if requested.
        if (required == MPI_THREAD_MULTIPLE)
            setenv("MV2_ENABLE_AFFINITY", "0", 1);
#endif

#if defined(MPICH) && defined(_POSIX_SOURCE)
        // This enables multi threading support in MPICH if requested.
        if (required == MPI_THREAD_MULTIPLE)
            setenv("MPICH_MAX_THREAD_SAFETY", "multiple", 1);
#endif
#endif

        int const retval =
            init(argc, argv, required, required, provided_threading_flag_);
        if (MPI_SUCCESS != retval && MPI_ERR_OTHER != retval)
        {
            // explicitly disable mpi if not run by mpirun
            rtcfg.add_entry("hpx.parcel.mpi.enable", "0");

            enabled_ = false;

            int msglen = 0;
            char message[MPI_MAX_ERROR_STRING + 1];
            MPI_Error_string(retval, message, &msglen);
            message[msglen] = '\0';

            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::util::mpi_environment::init",
                "MPI_Init_thread failed: {}.", message);
        }

        MPI_Comm_dup(MPI_COMM_WORLD, &communicator_);

        // explicitly disable multithreaded mpi if needed
        if (provided_threading_flag_ <= MPI_THREAD_SERIALIZED)
        {
            rtcfg.add_entry("hpx.parcel.mpi.multithreaded", "0");
        }

        if (provided_threading_flag_ == MPI_THREAD_FUNNELED)
        {
            enabled_ = false;
            has_called_init_ = false;
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::util::mpi_environment::init",
                "MPI_Init_thread: The underlying MPI implementation only "
                "supports MPI_THREAD_FUNNELED. This mode is not supported "
                "by HPX. Please pass "
                "--hpx:ini=hpx.parcel.mpi.multithreaded=0 to explicitly "
                "disable MPI multi-threading.");
        }

        // let all errors be returned from MPI calls
        MPI_Comm_set_errhandler(communicator_, MPI_ERRORS_RETURN);

        // initialize status
        this_rank = rank();

#if defined(HPX_HAVE_NETWORKING)
        if (this_rank == 0)
        {
            rtcfg.mode_ = hpx::runtime_mode::console;
        }
        else
        {
            rtcfg.mode_ = hpx::runtime_mode::worker;
        }
#elif defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
        rtcfg.mode_ = hpx::runtime_mode::console;
#else
        rtcfg.mode_ = hpx::runtime_mode::local;
#endif

        rtcfg.add_entry("hpx.parcel.mpi.rank", std::to_string(this_rank));
        rtcfg.add_entry("hpx.parcel.mpi.processorname", get_processor_name());

        scoped_lock l;

        void* max_tag_p = nullptr;
        int flag = 0;
        int const ret =
            MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, &max_tag_p, &flag);
        check_mpi_error(l, HPX_CURRENT_SOURCE_LOCATION(), ret);

        if (flag)
        {
            MPI_MAX_TAG =
                static_cast<int>(*static_cast<int*>(max_tag_p) & ~MPI_ACK_MASK);
        }
    }

    std::string mpi_environment::get_processor_name()
    {
        scoped_lock l;

        char name[MPI_MAX_PROCESSOR_NAME + 1] = {};
        int len = 0;
        MPI_Get_processor_name(name, &len);

        return {name};
    }

    void mpi_environment::finalize() noexcept
    {
        if (enabled() && has_called_init())
        {
            scoped_lock l;

            int is_finalized = 0;
            MPI_Finalized(&is_finalized);
            if (!is_finalized)
            {
                if (communicator_ != MPI_COMM_NULL)
                {
                    // Synchronize entry into MPI_Finalize across all ranks.
                    // The runtime shuts down independently on every
                    // locality, so ranks reach this point at noticeably
                    // different times. Entering MPI_Finalize while peers
                    // are still communicating exposes teardown races in
                    // the underlying MPI implementation (observed with
                    // Open MPI's btl/uct: fatal UD endpoint timeouts while
                    // a rank that is already inside MPI_Finalize processes
                    // a peer's late connection request). A barrier
                    // immediately before MPI_Finalize lets every rank
                    // finish its outstanding traffic while all peers still
                    // progress normally. Any error is ignored: proceeding
                    // to MPI_Finalize is the best remaining option either
                    // way.
                    [[maybe_unused]] int const ret = MPI_Barrier(communicator_);

                    // Release the duplicated communicator before
                    // MPI_Finalize reclaims the remaining MPI state.
                    MPI_Comm_free(&communicator_);
                }

                MPI_Finalize();
            }
        }
    }

    bool mpi_environment::enabled() noexcept
    {
        return enabled_;
    }

    bool mpi_environment::multi_threaded() noexcept
    {
        return provided_threading_flag_ > MPI_THREAD_SERIALIZED;
    }

    bool mpi_environment::has_called_init() noexcept
    {
        return has_called_init_;
    }

    int mpi_environment::size() noexcept
    {
        int res(-1);
        if (enabled())
        {
            scoped_lock l;
            MPI_Comm_size(communicator(), &res);
        }
        return res;
    }

    int mpi_environment::rank() noexcept
    {
        int res(-1);
        if (enabled())
        {
            scoped_lock l;
            MPI_Comm_rank(communicator(), &res);
        }
        return res;
    }

    MPI_Comm& mpi_environment::communicator() noexcept
    {
        return communicator_;
    }

    mpi_environment::scoped_lock::scoped_lock()
      : locked(true)
    {
        if (!multi_threaded())
        {
            mtx_.lock();
        }
    }

    mpi_environment::scoped_lock::~scoped_lock()
    {
        if (!multi_threaded())
        {
            locked = false;
            mtx_.unlock();
        }
    }

    void mpi_environment::scoped_lock::unlock()
    {
        if (!multi_threaded())
        {
            locked = false;
            mtx_.unlock();
        }
    }

    mpi_environment::scoped_try_lock::scoped_try_lock()
      : locked(true)
    {
        if (!multi_threaded())
        {
            locked = mtx_.try_lock();
        }
    }

    mpi_environment::scoped_try_lock::~scoped_try_lock()
    {
        if (!multi_threaded() && locked)
        {
            locked = false;
            mtx_.unlock();
        }
    }

    void mpi_environment::scoped_try_lock::unlock()
    {
        if (!multi_threaded() && locked)
        {
            locked = false;
            mtx_.unlock();
        }
    }

    namespace {

        [[noreturn]] void report_error(
            hpx::source_location const& sl, int error_code)
        {
            char error_string[MPI_MAX_ERROR_STRING + 1];
            int error_len = sizeof(error_string);
            int const ret =
                MPI_Error_string(error_code, error_string, &error_len);
            if (ret != MPI_SUCCESS)
            {
                HPX_THROW_EXCEPTION(hpx::error::internal_server_error,
                    sl.function_name(),
                    "MPI error (%s/%d): couldn't retrieve error string for "
                    "code %d",
                    sl.file_name(), sl.line(), error_code);
            }

            HPX_THROW_EXCEPTION(hpx::error::internal_server_error,
                sl.function_name(), "MPI error (%s/%d): %s", sl.file_name(),
                sl.line(), error_string);
        }
    }    // namespace

    void mpi_environment::check_mpi_error(
        scoped_lock& l, hpx::source_location const& sl, int error_code)
    {
        if (error_code == MPI_SUCCESS)
        {
            return;
        }

        l.unlock();

        report_error(sl, error_code);
    }

    void mpi_environment::check_mpi_error(
        scoped_try_lock& l, hpx::source_location const& sl, int error_code)
    {
        if (error_code == MPI_SUCCESS)
        {
            return;
        }

        l.unlock();

        report_error(sl, error_code);
    }

    // Acknowledgement: code adapted from github.com/jeffhammond/BigMPI
    MPI_Datatype mpi_environment::type_contiguous(size_t const nbytes)
    {
        constexpr int int_max = (std::numeric_limits<int>::max)();

        size_t c = nbytes / int_max;
        size_t r = nbytes % int_max;

        HPX_ASSERT(c < int_max);
        HPX_ASSERT(r < int_max);

        MPI_Datatype chunks;
        MPI_Type_vector(
            static_cast<int>(c), int_max, int_max, MPI_BYTE, &chunks);

        MPI_Datatype remainder;
        MPI_Type_contiguous(static_cast<int>(r), MPI_BYTE, &remainder);

        MPI_Aint const remdisp = static_cast<MPI_Aint>(c) * int_max;
        constexpr int blocklengths[2] = {1, 1};
        MPI_Aint displacements[2] = {0, remdisp};
        MPI_Datatype types[2] = {chunks, remainder};
        MPI_Datatype newtype;
        MPI_Type_create_struct(2, blocklengths, displacements, types, &newtype);

        MPI_Type_free(&chunks);
        MPI_Type_free(&remainder);

        return newtype;
    }

    MPI_Request mpi_environment::isend(
        void const* address, size_t size, int rank, int tag)
    {
        MPI_Request request;
        MPI_Datatype datatype;
        int length;
        if (size > static_cast<size_t>((std::numeric_limits<int>::max)()))
        {
            datatype = type_contiguous(size);
            MPI_Type_commit(&datatype);
            length = 1;
        }
        else
        {
            datatype = MPI_BYTE;
            length = static_cast<int>(size);
        }

        {
            scoped_lock l;
            int const ret = MPI_Isend(
                address, length, datatype, rank, tag, communicator(), &request);
            check_mpi_error(l, HPX_CURRENT_SOURCE_LOCATION(), ret);
        }

        if (datatype != MPI_BYTE)
            MPI_Type_free(&datatype);
        return request;
    }

    MPI_Request mpi_environment::irecv(
        void* address, size_t size, int rank, int tag)
    {
        MPI_Request request;
        MPI_Datatype datatype;
        int length;
        if (size > static_cast<size_t>((std::numeric_limits<int>::max)()))
        {
            datatype = type_contiguous(size);
            MPI_Type_commit(&datatype);
            length = 1;
        }
        else
        {
            datatype = MPI_BYTE;
            length = static_cast<int>(size);
        }

        {
            scoped_lock l;
            int const ret = MPI_Irecv(
                address, length, datatype, rank, tag, communicator(), &request);
            check_mpi_error(l, HPX_CURRENT_SOURCE_LOCATION(), ret);
        }

        if (datatype != MPI_BYTE)
            MPI_Type_free(&datatype);

        return request;
    }
}    // namespace hpx::util

#endif
