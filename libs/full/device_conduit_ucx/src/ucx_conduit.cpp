//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_DEVICE_CONDUIT_UCX)
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/plugin_factories.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/device_conduit_ucx/ucx_conduit.hpp>

#include <ucp/api/ucp.h>

#if defined(HPX_HAVE_CUDA)
#include <cuda.h>
#include <cuda_runtime_api.h>
#endif

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace hpx::parcelset::device_conduits {

    namespace {

        void check(ucs_status_t const status, char const* what)
        {
            if (status != UCS_OK)
            {
                HPX_THROW_EXCEPTION(hpx::error::network_error,
                    "hpx::parcelset::device_conduits::ucx_conduit",
                    "{} failed: {}", what, ucs_status_string(status));
            }
        }

        // the error of a UCX operation (from callbacks, which can't throw)
        std::exception_ptr make_error(
            ucs_status_t const status, char const* what)
        {
            return std::make_exception_ptr(hpx::exception(
                hpx::error::network_error,
                std::string("hpx::parcelset::device_conduits::ucx_conduit: ") +
                    what + " failed: " + ucs_status_string(status)));
        }

        // the UCX tag of a chunk: the sending locality and the descriptor's
        // tag (unique for it)
        constexpr std::uint64_t make_tag(
            serialization::detail::device_chunk_descriptor const& d) noexcept
        {
            return (d.source << 48) | (d.tag & ((std::uint64_t(1) << 48) - 1));
        }

        constexpr ucp_tag_t full_tag_mask = ~ucp_tag_t(0);

        // the completion of a send or receive (the 'user_data' of the UCX
        // request)
        struct send_state
        {
            hpx::move_only_function<void(std::exception_ptr)> sent;
        };

        struct recv_state
        {
            std::function<void(std::exception_ptr)> done;
        };

        void send_callback(
            void* request, ucs_status_t const status, void* user_data)
        {
            // done with UCX before the completion: once all sends have
            // completed, the conduit may be destroyed
            auto* state = static_cast<send_state*>(user_data);
            auto sent = HPX_MOVE(state->sent);
            delete state;
            ucp_request_free(request);

            // a failed send is reported (its parcel has left already)
            sent(status == UCS_OK ? nullptr :
                                    make_error(status, "ucp_tag_send_nbx"));
        }

        void recv_callback(void* request, ucs_status_t const status,
            ucp_tag_recv_info_t const*, void* user_data)
        {
            // done with UCX before the completion (see send_callback)
            auto* state = static_cast<recv_state*>(user_data);
            auto done = HPX_MOVE(state->done);
            delete state;
            ucp_request_free(request);

            // a failed receive fails the parcel (it isn't run)
            done(status == UCS_OK ? nullptr :
                                    make_error(status, "ucp_tag_recv_nbx"));
        }

        // a peer failed (with UCP_ERR_HANDLING_MODE_PEER): the transfers to
        // it fail and are reported, the endpoint isn't used successfully any
        // more
        void endpoint_error(
            void* arg, ucp_ep_h, ucs_status_t const status) noexcept
        {
            LPT_(warning).format(
                "ucx_conduit: the endpoint to locality {} failed: {}",
                static_cast<std::uint32_t>(
                    reinterpret_cast<std::uintptr_t>(arg)),
                ucs_status_string(status));
        }

        // UCX registers CUDA memory for the network (DMA-BUF) with the CUDA
        // context current on the calling OS thread, which an HPX worker thread
        // may not have: the GPU of the memory at 'p' (or -1)
        int device_of([[maybe_unused]] void const* p) noexcept
        {
#if defined(HPX_HAVE_CUDA)
            int device = -1;
            if (cuPointerGetAttribute(&device,
                    CU_POINTER_ATTRIBUTE_DEVICE_ORDINAL,
                    reinterpret_cast<CUdeviceptr>(p)) == CUDA_SUCCESS)
            {
                return device;
            }
#endif
            return -1;
        }

        // makes the primary context of 'device' current on this OS thread if
        // it has none (it stays current, there is nothing to restore)
        void ensure_context([[maybe_unused]] int const device) noexcept
        {
#if defined(HPX_HAVE_CUDA)
            CUcontext current = nullptr;
            if (device >= 0 && cuCtxGetCurrent(&current) == CUDA_SUCCESS &&
                current == nullptr && cudaSetDevice(device) != cudaSuccess)
            {
                (void) cudaGetLastError();
            }
#endif
        }

        // the conduit of this locality, if it is the UCX conduit
        ucx_conduit* this_conduit()
        {
            device_conduit* conduit = get_device_conduit();
            if (conduit == nullptr ||
                conduit->name() != ucx_conduit::name_impl())
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "hpx::parcelset::device_conduits::ucx_conduit",
                    "the UCX device conduit isn't used here (see "
                    "hpx.parcel.device_conduit)");
            }
            return static_cast<ucx_conduit*>(conduit);
        }
    }    // namespace

    // the worker address of this locality (for the endpoints of the others)
    std::vector<char> ucx_conduit_worker_address()
    {
        return this_conduit()->worker_address();
    }
}    // namespace hpx::parcelset::device_conduits

HPX_PLAIN_ACTION(hpx::parcelset::device_conduits::ucx_conduit_worker_address,
    ucx_conduit_worker_address_action)

namespace hpx::parcelset::device_conduits {

    ///////////////////////////////////////////////////////////////////////////
    void ucx_conduit::config_defaults(std::vector<std::string>& ini)
    {
        ini.emplace_back("[hpx.parcel.ucx_conduit]");
        // UCX's rendezvous (zero-copy) protocol from this size on, unless
        // HPX_DEVICE_CONDUIT_UCX_RNDV_THRESH is set
        ini.emplace_back(
            "rndv_thresh = ${HPX_PARCEL_UCX_CONDUIT_RNDV_THRESH:16384}");
    }

    ucx_conduit::ucx_conduit(util::runtime_configuration const& cfg)
    {
        // the conduit's UCX configuration: UCX_* (shared with a UCX based
        // MPI library), HPX_DEVICE_CONDUIT_UCX_* for the conduit only (UCX
        // reads the settings of memory domains and transports from UCX_*
        // only)
        ucp_config_t* config = nullptr;
        check(ucp_config_read("HPX_DEVICE_CONDUIT", nullptr, &config),
            "ucp_config_read");

        if (std::getenv("HPX_DEVICE_CONDUIT_UCX_RNDV_THRESH") == nullptr)
        {
            std::string const rndv_thresh =
                cfg.get_entry("hpx.parcel.ucx_conduit.rndv_thresh", "16384");
            ucs_status_t const status =
                ucp_config_modify(config, "RNDV_THRESH", rndv_thresh.c_str());
            if (status != UCS_OK)
            {
                ucp_config_release(config);
                check(status, "ucp_config_modify(RNDV_THRESH)");
            }
        }

        ucp_params_t params;
        std::memset(&params, 0, sizeof(params));
        params.field_mask = UCP_PARAM_FIELD_FEATURES;
        params.features = UCP_FEATURE_TAG;

        ucs_status_t const status = ucp_init(&params, config, &context_);
        ucp_config_release(config);
        check(status, "ucp_init");

        // HPX threads send and receive from any OS thread
        ucp_worker_params_t worker_params;
        std::memset(&worker_params, 0, sizeof(worker_params));
        worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
        worker_params.thread_mode = UCS_THREAD_MODE_MULTI;
        if (ucs_status_t const s =
                ucp_worker_create(context_, &worker_params, &worker_);
            s != UCS_OK)
        {
            ucp_cleanup(context_);
            check(s, "ucp_worker_create");
        }

        // UCX may provide less than requested (a UCX built without
        // multithreading support)
        ucp_worker_attr_t worker_attr;
        std::memset(&worker_attr, 0, sizeof(worker_attr));
        worker_attr.field_mask = UCP_WORKER_ATTR_FIELD_THREAD_MODE;
        if (ucs_status_t const s = ucp_worker_query(worker_, &worker_attr);
            s != UCS_OK || worker_attr.thread_mode != UCS_THREAD_MODE_MULTI)
        {
            ucp_worker_destroy(worker_);
            ucp_cleanup(context_);
            check(s != UCS_OK ? s : UCS_ERR_UNSUPPORTED,
                "ucp_worker_create (UCS_THREAD_MODE_MULTI, a UCX built with "
                "--enable-mt is required)");
        }

        ucp_address_t* address = nullptr;
        std::size_t address_length = 0;
        if (ucs_status_t const s =
                ucp_worker_get_address(worker_, &address, &address_length);
            s != UCS_OK)
        {
            ucp_worker_destroy(worker_);
            ucp_cleanup(context_);
            check(s, "ucp_worker_get_address");
        }
        auto const* begin = reinterpret_cast<char const*>(address);
        address_.assign(begin, begin + address_length);
        ucp_worker_release_address(worker_, address);
    }

    ucx_conduit::~ucx_conduit()
    {
        // The conduit is destroyed only after drain() has seen all sends
        // finish, and a send finishes only after its endpoint's future has
        // become ready (the task creating the endpoint has run): no task
        // calls UCX or accesses the conduit any more (asserted below). The
        // peers may have shut down already: close the endpoints without
        // flushing them.
        std::vector<ucs_status_ptr_t> closing;
        for (auto& [locality_id, ep] : endpoints_)
        {
            HPX_ASSERT(ep.is_ready());
            if (!ep.is_ready() || ep.has_exception())
            {
                continue;
            }

            ucp_request_param_t param;
            std::memset(&param, 0, sizeof(param));
            param.op_attr_mask = UCP_OP_ATTR_FIELD_FLAGS;
            param.flags = UCP_EP_CLOSE_FLAG_FORCE;
            ucs_status_ptr_t const request = ucp_ep_close_nbx(ep.get(), &param);
            if (UCS_PTR_IS_ERR(request))
            {
                LPT_(warning).format("ucx_conduit: closing the endpoint to "
                                     "locality {} failed: {}",
                    locality_id, ucs_status_string(UCS_PTR_STATUS(request)));
            }
            else if (request != nullptr)
            {
                closing.push_back(request);
            }
        }
        for (ucs_status_ptr_t const request : closing)
        {
            ucs_status_t status;
            while (
                (status = ucp_request_check_status(request)) == UCS_INPROGRESS)
            {
                ucp_worker_progress(worker_);
            }
            if (status != UCS_OK)
            {
                LPT_(warning).format(
                    "ucx_conduit: closing an endpoint failed: {}",
                    ucs_status_string(status));
            }
            ucp_request_free(request);
        }
        endpoints_.clear();

        ucp_worker_destroy(worker_);
        ucp_cleanup(context_);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::shared_future<ucp_ep*> ucx_conduit::endpoint(
        std::uint32_t const locality_id)
    {
        std::lock_guard<hpx::spinlock> l(endpoints_mtx_);
        if (auto const it = endpoints_.find(locality_id);
            it != endpoints_.end())
        {
            return it->second;
        }

        // the first message to this locality: fetch the address of its
        // worker (on an HPX thread, not in the serialization of a parcel)
        hpx::shared_future<ucp_ep*> ep =
            hpx::async([this, locality_id]() -> ucp_ep* {
                std::vector<char> const address =
                    hpx::async(ucx_conduit_worker_address_action(),
                        naming::get_id_from_locality_id(locality_id))
                        .get();

                ucp_ep_params_t params;
                std::memset(&params, 0, sizeof(params));
                // the error mode the forced close at shutdown needs (both
                // sides use it)
                params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS |
                    UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE |
                    UCP_EP_PARAM_FIELD_ERR_HANDLER;
                params.err_mode = UCP_ERR_HANDLING_MODE_PEER;
                params.err_handler.cb = &endpoint_error;
                params.err_handler.arg = reinterpret_cast<void*>(
                    static_cast<std::uintptr_t>(locality_id));
                params.address =
                    reinterpret_cast<ucp_address_t const*>(address.data());

                ucp_ep_h ep = nullptr;
                check(ucp_ep_create(worker_, &params, &ep), "ucp_ep_create");
                return ep;
            });
        endpoints_.emplace(locality_id, ep);
        return ep;
    }

    void ucx_conduit::note_device(void const* p) noexcept
    {
        int const device = device_of(p);
        if (device >= 0)
        {
            ensure_context(device);
            device_.store(device, std::memory_order_relaxed);
        }
    }

    void ucx_conduit::start_send(ucp_ep* ep, std::uint64_t const tag,
        void const* p, std::size_t const size,
        hpx::move_only_function<void(std::exception_ptr)> sent)
    {
        note_device(p);

        auto* state = new send_state{HPX_MOVE(sent)};

        ucp_request_param_t param;
        std::memset(&param, 0, sizeof(param));
        param.op_attr_mask =
            UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_USER_DATA;
        param.cb.send = &send_callback;
        param.user_data = state;

        ucs_status_ptr_t const request =
            ucp_tag_send_nbx(ep, p, size, tag, &param);
        if (request == nullptr)
        {
            // sent right away, the callback isn't called
            state->sent(nullptr);
            delete state;
        }
        else if (UCS_PTR_IS_ERR(request))
        {
            state->sent(
                make_error(UCS_PTR_STATUS(request), "ucp_tag_send_nbx"));
            delete state;
        }
    }

    void ucx_conduit::send_impl(std::uint32_t const locality_id,
        serialization::detail::device_chunk_descriptor const& d, void const* p,
        std::size_t const size,
        hpx::move_only_function<void(std::exception_ptr)> sent)
    {
        std::uint64_t const tag = make_tag(d);
        hpx::shared_future<ucp_ep*> ep = endpoint(locality_id);
        if (ep.is_ready())
        {
            start_send(ep.get(), tag, p, size, HPX_MOVE(sent));
            return;
        }

        // send once connected (the future of the continuation isn't needed)
        hpx::future<void> const sending =
            ep.then([this, tag, p, size, sent = HPX_MOVE(sent)](
                        hpx::shared_future<ucp_ep*>&& f) mutable {
                if (f.has_exception())
                {
                    // the data can't be sent, release the message anyway
                    sent(f.get_exception_ptr());
                    return;
                }
                start_send(f.get(), tag, p, size, HPX_MOVE(sent));
            });
        HPX_UNUSED(sending);
    }

    void ucx_conduit::recv_impl(
        serialization::detail::device_chunk_descriptor const& d, void* p,
        std::size_t const size, std::function<void(std::exception_ptr)> done)
    {
        note_device(p);

        auto* state = new recv_state{HPX_MOVE(done)};

        ucp_request_param_t param;
        std::memset(&param, 0, sizeof(param));
        param.op_attr_mask =
            UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_USER_DATA;
        param.cb.recv = &recv_callback;
        param.user_data = state;

        ucs_status_ptr_t const request = ucp_tag_recv_nbx(
            worker_, p, size, make_tag(d), full_tag_mask, &param);
        if (request == nullptr)
        {
            // received right away, the callback isn't called
            state->done(nullptr);
            delete state;
        }
        else if (UCS_PTR_IS_ERR(request))
        {
            state->done(
                make_error(UCS_PTR_STATUS(request), "ucp_tag_recv_nbx"));
            delete state;
        }
    }

    bool ucx_conduit::background_work_impl()
    {
        std::unique_lock<hpx::spinlock> l(progress_mtx_, std::try_to_lock);
        if (!l.owns_lock())
        {
            return false;
        }

        // progress registers the memory of rendezvous messages
        ensure_context(device_.load(std::memory_order_relaxed));
        return ucp_worker_progress(worker_) != 0;
    }
}    // namespace hpx::parcelset::device_conduits

HPX_REGISTER_DEVICE_CONDUIT(hpx::parcelset::device_conduits::ucx_conduit, ucx)

#endif
