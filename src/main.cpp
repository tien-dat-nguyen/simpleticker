//
// Copyright (c) 2020 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "config.hpp"
#include "websocket/connection.hpp"

#include <cstdio>
#include <iostream>

net::awaitable< void >
bnbbtc_miniticker()
{
    // expect that connect_options.headers being not empty, so put an arbitrary one.
    websocket::connect_options options{};
    options.headers.insert(boost::beast::http::field::set_cookie, "true");

    auto ws = co_await websocket::connect("wss://stream.binance.com:9443/ws/bnbbtc@miniTicker", options);

    for (;;)
    {
        auto event = co_await ws.consume();
        if (event.is_error())
        {
            if (event.error() == beast::websocket::error::closed)
                std::cerr << "peer closed connection: " << ws.reason()
                          << std::endl;
            else
                std::cerr << "connection error: " << event.error() <<
                std::endl;
            break;
        }
        else
        {
            std::cout << "message received: " << event.message() <<
            std::endl;
        }
    }
}

int
main()
{
    net::io_context ioctx;

    net::co_spawn(
        ioctx.get_executor(), [] { return bnbbtc_miniticker(); }, net::detached);

    ioctx.run();
}
