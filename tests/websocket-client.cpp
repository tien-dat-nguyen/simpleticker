#include "../src/websocket-client.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <string>

BOOST_AUTO_TEST_SUITE(simple_ticker);

BOOST_AUTO_TEST_CASE(class_websocket_client)
{
    const std::string url{"echo.websocket.org"};
    const std::string endpoint {"/"};
    const std::string port{"443"};
    const std::string message{"a dummy ticker"};

    boost::asio::ssl::context ctx{net::ssl::context::tls_client};

    boost::asio::io_context ioc;

    using simpleticker::websocket_client;

    websocket_client client(url, endpoint, port, ioc, ctx);

    bool connected{false};
    bool messageSent{false};
    bool messageReceived{false};
    bool disconnected{false};
    std::string echo;

    auto onSend = [&messageSent](auto ec) {
        messageSent = !ec;
    };

    auto onConnect = [&](auto ec) {
        connected = !ec;
        if (!ec)
        {
            client.Send(message, onSend);
        }
    };

    auto onClose = [&](auto ec) {
        disconnected = !ec;
    };

    auto onReceived = [&](auto ec, auto received) {
        messageReceived = !ec;
        echo = std::move(received);
        client.Close(onClose);
    };

    client.Connect(onConnect, onReceived);
    ioc.run();

    BOOST_CHECK(connected);
    BOOST_CHECK(messageSent);
    BOOST_CHECK(messageReceived);
    BOOST_CHECK(disconnected);
    BOOST_CHECK_EQUAL(message, echo);
}

BOOST_AUTO_TEST_SUITE_END();