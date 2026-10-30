//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  Parts of this code were inspired by https://github.com/josuttis/jthread. The
//  original code was published by Nicolai Josuttis and Lewis Baker under the
//  Creative Commons Attribution 4.0 International License
//  (http://creativecommons.org/licenses/by/4.0/).

#include <hpx/init.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/testing.hpp>

#include <chrono>
#include <utility>

void test_stop_token_basic_api()
{
    // create stop_source
    hpx::stop_source ssrc;
    HPX_TEST(ssrc.stop_possible());
    HPX_TEST(!ssrc.stop_requested());

    // create stop_token from stop_source
    hpx::stop_token stok{ssrc.get_token()};
    HPX_TEST(ssrc.stop_possible());
    HPX_TEST(!ssrc.stop_requested());
    HPX_TEST(stok.stop_possible());
    HPX_TEST(!stok.stop_requested());

    // register callback
    bool cb1called{false};
    auto cb1 = [&] { cb1called = true; };
    {
        hpx::stop_callback<decltype(cb1)> scb1(stok, cb1);    // copies cb1
        HPX_TEST(ssrc.stop_possible());
        HPX_TEST(!ssrc.stop_requested());
        HPX_TEST(stok.stop_possible());
        HPX_TEST(!stok.stop_requested());
        HPX_TEST(!cb1called);
    }    // unregister callback

    // register another callback
    bool cb2called{false};
    auto cb2 = [&] {
        HPX_TEST(stok.stop_requested());
        cb2called = true;
    };

    hpx::stop_callback<decltype(cb2)> scb2a{stok, cb2};    // copies cb2
    hpx::stop_callback<decltype(cb2)> scb2b{stok, std::move(cb2)};
    HPX_TEST(ssrc.stop_possible());
    HPX_TEST(!ssrc.stop_requested());
    HPX_TEST(stok.stop_possible());
    HPX_TEST(!stok.stop_requested());
    HPX_TEST(!cb1called);
    HPX_TEST(!cb2called);

    // request stop
    auto b = ssrc.request_stop();
    HPX_TEST(b);
    HPX_TEST(ssrc.stop_possible());
    HPX_TEST(ssrc.stop_requested());
    HPX_TEST(stok.stop_possible());
    HPX_TEST(stok.stop_requested());
    HPX_TEST(!cb1called);
    HPX_TEST(cb2called);

    b = ssrc.request_stop();
    HPX_TEST(!b);

    // register another callback
    bool cb3called{false};

    auto cb3 = [&] { cb3called = true; };
    hpx::stop_callback<decltype(cb3)> scb3(stok, std::move(cb3));
    HPX_TEST(ssrc.stop_possible());
    HPX_TEST(ssrc.stop_requested());
    HPX_TEST(stok.stop_possible());
    HPX_TEST(stok.stop_requested());
    HPX_TEST(!cb1called);
    HPX_TEST(cb2called);
    HPX_TEST(cb3called);
}

///////////////////////////////////////////////////////////////////////////////
void test_stop_token_api()
{
    // stop_source: create, copy, assign and destroy
    {
        hpx::stop_source is1;
        hpx::stop_source is2{is1};
        hpx::stop_source is3 = is1;
        hpx::stop_source is4{std::move(is1)};
        // NOLINTNEXTLINE(bugprone-use-after-move)
        HPX_TEST(!is1.stop_possible());
        HPX_TEST(is2.stop_possible());
        HPX_TEST(is3.stop_possible());
        HPX_TEST(is4.stop_possible());
        is1 = is2;
        HPX_TEST(is1.stop_possible());
        is1 = std::move(is2);
        // NOLINTNEXTLINE(bugprone-use-after-move)
        HPX_TEST(!is2.stop_possible());
        std::swap(is1, is2);
        HPX_TEST(!is1.stop_possible());
        HPX_TEST(is2.stop_possible());
        is1.swap(is2);
        HPX_TEST(is1.stop_possible());
        HPX_TEST(!is2.stop_possible());

        // stop_source without shared stop state
        hpx::stop_source is0{hpx::nostopstate};
        HPX_TEST(!is0.stop_requested());
        HPX_TEST(!is0.stop_possible());
    }

    // stop_token: create, copy, assign and destroy
    {
        hpx::stop_token it1;
        hpx::stop_token it2{it1};
        hpx::stop_token it3 = it1;
        hpx::stop_token it4{std::move(it1)};
        it1 = it2;
        it1 = std::move(it2);
        std::swap(it1, it2);
        it1.swap(it2);
        HPX_TEST(!it1.stop_possible());
        HPX_TEST(!it2.stop_possible());
        HPX_TEST(!it3.stop_possible());
        HPX_TEST(!it4.stop_possible());
    }

    // tokens without an source are no longer interruptible
    {
        hpx::stop_source* isp = new hpx::stop_source;
        hpx::stop_source& isr = *isp;
        hpx::stop_token it{isr.get_token()};
        HPX_TEST(isr.stop_possible());
        HPX_TEST(it.stop_possible());
        delete isp;    // not interrupted and losing last source
        HPX_TEST(!it.stop_possible());
    }

    {
        hpx::stop_source* isp = new hpx::stop_source;
        hpx::stop_source& isr = *isp;
        hpx::stop_token it{isr.get_token()};
        HPX_TEST(isr.stop_possible());
        HPX_TEST(it.stop_possible());
        isr.request_stop();
        delete isp;    // interrupted and losing last source
        HPX_TEST(it.stop_possible());
    }

    // stop_possible(), stop_requested(), and request_stop()
    {
        hpx::stop_source is_not_valid;
        hpx::stop_source is_not_stopped{std::move(is_not_valid)};
        hpx::stop_source is_stopped;
        is_stopped.request_stop();

        // NOLINTNEXTLINE(bugprone-use-after-move)
        hpx::stop_token it_not_valid{is_not_valid.get_token()};
        hpx::stop_token it_not_stopped{is_not_stopped.get_token()};
        hpx::stop_token it_stopped{is_stopped.get_token()};

        // stop_possible() and stop_requested()
        HPX_TEST(!is_not_valid.stop_possible());
        HPX_TEST(is_not_stopped.stop_possible());
        HPX_TEST(is_stopped.stop_possible());
        HPX_TEST(!is_not_valid.stop_requested());
        HPX_TEST(!is_not_stopped.stop_requested());
        HPX_TEST(is_stopped.stop_requested());

        // stop_possible() and stop_requested()
        HPX_TEST(!it_not_valid.stop_possible());
        HPX_TEST(it_not_stopped.stop_possible());
        HPX_TEST(it_stopped.stop_possible());
        HPX_TEST(!it_not_stopped.stop_requested());
        HPX_TEST(it_stopped.stop_requested());

        // request_stop()
        HPX_TEST(is_not_stopped.request_stop() == true);
        HPX_TEST(is_not_stopped.request_stop() == false);
        HPX_TEST(is_stopped.request_stop() == false);
        HPX_TEST(is_not_stopped.stop_requested());
        HPX_TEST(is_stopped.stop_requested());
        HPX_TEST(it_not_stopped.stop_requested());
        HPX_TEST(it_stopped.stop_requested());
    }

    // assignment and swap()
    {
        hpx::stop_source is_not_valid;
        hpx::stop_source is_not_stopped{std::move(is_not_valid)};
        hpx::stop_source is_stopped;
        is_stopped.request_stop();

        // NOLINTNEXTLINE(bugprone-use-after-move)
        hpx::stop_token it_not_valid{is_not_valid.get_token()};
        hpx::stop_token it_not_stopped{is_not_stopped.get_token()};
        hpx::stop_token it_stopped{is_stopped.get_token()};

        // assignments and swap()
        HPX_TEST(!hpx::stop_token{}.stop_requested());
        it_stopped = hpx::stop_token{};
        HPX_TEST(!it_stopped.stop_possible());
        HPX_TEST(!it_stopped.stop_requested());
        is_stopped = hpx::stop_source{};
        HPX_TEST(is_stopped.stop_possible());
        HPX_TEST(!is_stopped.stop_requested());

        std::swap(it_stopped, it_not_valid);
        HPX_TEST(!it_stopped.stop_possible());
        HPX_TEST(!it_not_valid.stop_possible());
        HPX_TEST(!it_not_valid.stop_requested());
        hpx::stop_token itnew = std::move(it_not_valid);
        // NOLINTNEXTLINE(bugprone-use-after-move)
        HPX_TEST(!it_not_valid.stop_possible());

        std::swap(is_stopped, is_not_valid);
        HPX_TEST(!is_stopped.stop_possible());
        HPX_TEST(is_not_valid.stop_possible());
        HPX_TEST(!is_not_valid.stop_requested());
        hpx::stop_source isnew = std::move(is_not_valid);
        // NOLINTNEXTLINE(bugprone-use-after-move)
        HPX_TEST(!is_not_valid.stop_possible());
    }

    // shared ownership semantics
    hpx::stop_source is;
    hpx::stop_token it1{is.get_token()};
    hpx::stop_token it2{it1};
    HPX_TEST(is.stop_possible() && !is.stop_requested());
    HPX_TEST(it1.stop_possible() && !it1.stop_requested());
    HPX_TEST(it2.stop_possible() && !it2.stop_requested());
    is.request_stop();
    HPX_TEST(is.stop_possible() && is.stop_requested());
    HPX_TEST(it1.stop_possible() && it1.stop_requested());
    HPX_TEST(it2.stop_possible() && it2.stop_requested());

    // == and !=
    {
        hpx::stop_source is_not_valid1;
        hpx::stop_source is_not_valid2;
        hpx::stop_source is_not_stopped1{std::move(is_not_valid1)};
        // NOLINTNEXTLINE(bugprone-use-after-move)
        hpx::stop_source is_not_stopped2{is_not_stopped1};
        hpx::stop_source is_stopped1{std::move(is_not_valid2)};
        // NOLINTNEXTLINE(bugprone-use-after-move)
        hpx::stop_source is_stopped2{is_stopped1};
        is_stopped1.request_stop();

        // NOLINTNEXTLINE(bugprone-use-after-move)
        hpx::stop_token it_not_valid1{is_not_valid1.get_token()};
        // NOLINTNEXTLINE(bugprone-use-after-move)
        hpx::stop_token it_not_valid2{is_not_valid2.get_token()};
        hpx::stop_token it_not_valid3;
        hpx::stop_token it_not_stopped1{is_not_stopped1.get_token()};
        hpx::stop_token it_not_stopped2{is_not_stopped2.get_token()};
        hpx::stop_token it_not_stopped3{it_not_stopped1};
        hpx::stop_token it_stopped1{is_stopped1.get_token()};
        hpx::stop_token it_stopped2{is_stopped2.get_token()};
        hpx::stop_token it_stopped3{it_stopped2};

        HPX_TEST(is_not_valid1 == is_not_valid2);
        HPX_TEST(is_not_stopped1 == is_not_stopped2);
        HPX_TEST(is_stopped1 == is_stopped2);
        HPX_TEST(is_not_valid1 != is_not_stopped1);
        HPX_TEST(is_not_valid1 != is_stopped1);
        HPX_TEST(is_not_stopped1 != is_stopped1);

        HPX_TEST(it_not_valid1 == it_not_valid2);
        HPX_TEST(it_not_valid2 == it_not_valid3);
        HPX_TEST(it_not_stopped1 == it_not_stopped2);
        HPX_TEST(it_not_stopped2 == it_not_stopped3);
        HPX_TEST(it_stopped1 == it_stopped2);
        HPX_TEST(it_stopped2 == it_stopped3);
        HPX_TEST(it_not_valid1 != it_not_stopped1);
        HPX_TEST(it_not_valid1 != it_stopped1);
        HPX_TEST(it_not_stopped1 != it_stopped1);

        HPX_TEST(!(is_not_valid1 != is_not_valid2));
        HPX_TEST(!(it_not_valid1 != it_not_valid2));
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename D>
void sleep(D duration)
{
    if (duration > std::chrono::milliseconds{0})
    {
        std::this_thread::sleep_for(duration);
    }
}

template <typename D>
void test_stoken(D duration)
{
    int okSteps = 0;
    try
    {
        hpx::stop_token it0;    // should not allocate anything

        hpx::stop_source interruptor;
        hpx::stop_token interruptee{interruptor.get_token()};
        ++okSteps;
        sleep(duration);    // 1
        HPX_TEST(!interruptor.stop_requested());
        HPX_TEST(!interruptee.stop_requested());

        interruptor.request_stop();    // INTERRUPT !!!
        ++okSteps;
        sleep(duration);    // 2
        HPX_TEST(interruptor.stop_requested());
        HPX_TEST(interruptee.stop_requested());

        interruptor.request_stop();
        ++okSteps;
        sleep(duration);    // 3
        HPX_TEST(interruptor.stop_requested());
        HPX_TEST(interruptee.stop_requested());

        interruptor = hpx::stop_source{};
        interruptee = interruptor.get_token();
        ++okSteps;
        sleep(duration);    // 4
        HPX_TEST(!interruptor.stop_requested());
        HPX_TEST(!interruptee.stop_requested());

        interruptor.request_stop();    // INTERRUPT !!!
        ++okSteps;
        sleep(duration);    // 5
        HPX_TEST(interruptor.stop_requested());
        HPX_TEST(interruptee.stop_requested());

        interruptor.request_stop();
        ++okSteps;
        sleep(duration);    // 6
        HPX_TEST(interruptor.stop_requested());
        HPX_TEST(interruptee.stop_requested());
    }
    catch (...)
    {
        HPX_TEST(false);
    }
    HPX_TEST(okSteps == 6);
}

void test_stop_callback_without_stop_source()
{
    hpx::stop_token stok;
    {
        hpx::stop_source ssrc;
        stok = ssrc.get_token();
    }
    HPX_TEST(!stok.stop_possible());

    bool cb1called{false};
    auto cb1 = [&] { cb1called = true; };
    {
        hpx::stop_callback<decltype(cb1)> scb1(stok, cb1);
    }
    HPX_TEST(!cb1called);

    bool cb2called{false};
    auto cb2 = [&] { cb2called = true; };
    {
        hpx::stop_callback<decltype(cb2)> scb2(std::move(stok), cb2);
    }
    HPX_TEST(!cb2called);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_stop_token_basic_api();
    test_stop_token_api();
    test_stop_callback_without_stop_source();
    test_stoken(std::chrono::seconds{0});
    test_stoken(std::chrono::milliseconds{500});

    hpx::local::finalize();
    return hpx::util::report_errors();
}

int main(int argc, char* argv[])
{
    return hpx::local::init(hpx_main, argc, argv);
}
