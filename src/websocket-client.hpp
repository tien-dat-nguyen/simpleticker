#ifndef SIMPLE_TICKER_WEBSOCKET_CLIENT_H
#define SIMPLE_TICKER_WEBSOCKET_CLIENT_H

#include "config.hpp"

#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>

namespace simpleticker
{

    // Make it testable as we can mock Resover and WebSocketStream
    template <
        typename Resolver,
        typename WebSocketStream>
    class websocket_client_t
    {
    public:
        websocket_client_t(
        const std::string& url,
        const std::string& endpoint,
        const std::string& port,
        net::io_context& ioc,
        net::ssl::context& ctx) : url_(url), endpoint_(endpoint), port_(port), 
        /*resolver_(boost::asio::make_strand(ioc)),*/ ws_(boost::asio::make_strand(ioc), ctx) 
        { }

        ~websocket_client_t();

        net::awaitable<void> Connect(
            std::function<void(error_code)> onConnect = nullptr,
            std::function<void(error_code,
                               std::string &&)>
                onMessage = nullptr,
            std::function<void(error_code)> onDisconnect = nullptr) {
                // Save the user callbacks for later use.
                onConnect_ = onConnect;
                onMessage_ = onMessage;
                onDisconnect_ = onDisconnect;

            // Start the chain of asynchronous callbacks.
            // resolver_.async_resolve(url_, port_,
            //     [this](auto ec, auto endpoint) {
            //         OnResolve(ec, endpoint);
            //     });

            // use coroutine instead, avoid the callback hell above.
            auto resolve_result = co_await ResolveAsync(url_, port_);
            co_await ConnectTCPAsync(resolve_result);
            co_await HandshakeTlsAsync();
            co_await HandshakeWsAsync();
        }

        net::awaitable<std::string> Consume() {
            auto [ec, len] = co_await ReadAsync();
            if (ec)
            {
                co_return std::string{};
            }
            else
            {
                std::string message {beast::buffers_to_string(rBuffer_.data())};
                rBuffer_.consume(len);
                co_return message;
            }
        }


        void Send(
            const std::string &message,
            std::function<void(error_code)> onSend = nullptr) {
            ws_.async_write(boost::asio::buffer(std::move(message)),
            [this, onSend](auto ec, auto) {
                if (onSend) {
                    onSend(ec);
                }
            });
        }

        void Close(
            std::function<void(error_code)> onClose = nullptr) {
            ws_.async_close(
            boost::beast::websocket::close_code::none,
            [this, onClose](auto ec) {
                if (onClose) {
                    onClose(ec);
                }
            });
        }

    private:
        std::string url_;
        std::string endpoint_;
        std::string port_;

        // Resolver resolver_;
        WebSocketStream ws_;

        beast::flat_buffer rBuffer_;

        std::function<void(error_code)> onConnect_{nullptr};
        std::function<void(error_code,
                           std::string &&)> onMessage_{nullptr};
        std::function<void(error_code)> onDisconnect_{nullptr};

        static void Log(
        const std::string& where,
        error_code ec){
            std::cerr << "[" << std::setw(20) << where << "] "
                  << (ec ? "Error: " : "OK")
                  << (ec ? ec.message() : "")
                  << std::endl;
        }

        net::awaitable<net::ip::tcp::resolver::results_type> ResolveAsync(std::string const& host,
            std::string const& port)
        {
            auto resolver =
                Resolver{co_await net::this_coro::executor};
            co_return co_await resolver.async_resolve(
                host, port, net::use_awaitable);
        }

        void OnResolve(
            const error_code &ec,
            net::ip::tcp::resolver::iterator endpoint) {
            if (ec) {
                Log("OnResolve", ec);
                if (onConnect_) {
                    onConnect_(ec);
                }
                return;
            }

            beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(5));

            beast::get_lowest_layer(ws_).async_connect(*endpoint,
                [this](auto ec) {
                    OnConnect(ec);
                });
        }

        net::awaitable<void> ConnectTCPAsync(net::ip::tcp::resolver::results_type results)
        {
            auto &tcp_layer = beast::get_lowest_layer(ws_);

            tcp_layer.expires_after(std::chrono::seconds(5));

            auto ep =  co_await tcp_layer.async_connect(results, net::use_awaitable);
            boost::ignore_unused(ep);
        }

        void OnConnect(const error_code &ec) {
            if (ec) {
                Log("OnConnect", ec);
                if (onConnect_) {
                    onConnect_(ec);
                }
                return;
            }

            beast::get_lowest_layer(ws_).expires_never();
            ws_.set_option(
                beast::websocket::stream_base::timeout::suggested(
                beast::role_type::client));

        
            ws_.next_layer().async_handshake(net::ssl::stream_base::client,
                net::use_awaitable);
        }

        net::awaitable<void> HandshakeTlsAsync()
        {
            beast::get_lowest_layer(ws_).expires_never();
            ws_.set_option(
                beast::websocket::stream_base::timeout::suggested(
                beast::role_type::client));

        
            co_await ws_.next_layer().async_handshake(net::ssl::stream_base::client,
                net::use_awaitable);
        }

        void OnTlsHandshake(const error_code &ec) {
            if (ec) {
                Log("OnTlsHandshake", ec);
                if (onConnect_) {
                    onConnect_(ec);
                }
                return;
            }

            ws_.async_handshake(url_, endpoint_,
                [this](auto ec) {
                    OnHandshake(ec);
                });
        }

        net::awaitable<void> HandshakeWsAsync() {
            beast::websocket::response_type response;
            co_await ws_.async_handshake(response, url_, endpoint_, net::use_awaitable);
        }

        void OnHandshake(
            const error_code &ec) {
            if (ec) {
                Log("OnHandshake", ec);
                if (onConnect_) {
                    onConnect_(ec);
                }
                return;
            }

            ws_.text(true);

            ConsumeMessage(ec);

            if (onConnect_) {
                onConnect_(ec);
            }
        }

        void ConsumeMessage(
            const error_code &ec) {
            if (ec == boost::asio::error::operation_aborted) {
                if (onDisconnect_) {
                    onDisconnect_(ec);
                }   
                return;
            }

            ws_.async_read(rBuffer_,
                [this](auto ec, auto nBytes) {
                    OnRead(ec, nBytes);
                    ConsumeMessage(ec);
                });
        }

        net::awaitable<std::pair<error_code, std::size_t>> ReadAsync() {
            error_code ec;
            auto size = co_await ws_.async_read(rBuffer_, net::redirect_error(net::use_awaitable, ec));
            co_return std::make_pair(ec, size);
        }

        void OnRead(
            const error_code &ec,
            size_t nBytes) {
            if (ec) {
                return;
            }

            std::string message {beast::buffers_to_string(rBuffer_.data())};
            rBuffer_.consume(nBytes);
            if (onMessage_) {
                onMessage_(ec, std::move(message));
            }
        }
    };

    using websocket_client = websocket_client_t<
    net::ip::tcp::resolver,
    beast::websocket::stream<
        beast::ssl_stream<beast::tcp_stream>
    >
>;

} // namespace simpleticker

#endif // SIMPLE_TICKER_WEBSOCKET_CLIENT_H