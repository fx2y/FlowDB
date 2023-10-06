#include "connection_pool.h"
#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

void
timer_handler(const boost::system::error_code &ec, boost::asio::steady_timer &timer, ConnectionPool &connection_pool) {
    if (!ec) {
        connection_pool.detect_failures();
        timer.expires_at(timer.expiry() + boost::asio::chrono::seconds(5));
        timer.async_wait([&](const boost::system::error_code &ec) {
            timer_handler(ec, timer, connection_pool);
        });
    }
}

int main() {
    boost::asio::io_context io_context;
    // Define the endpoints for the network layer
    std::vector<tcp::endpoint> endpoints = {
            tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8000),
            tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8001),
            tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8002)
    };

    // Create a connection pool with 10 connections
    ConnectionPool connection_pool(io_context, endpoints, 5);
    connection_pool.resize(10);

    // Start a timer to call detect_failures every 5 seconds
    boost::asio::steady_timer timer(io_context, boost::asio::chrono::seconds(5));
    timer.async_wait([&](const boost::system::error_code &ec) {
        timer_handler(ec, timer, connection_pool);
    });

    try {
        tcp::endpoint local_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 0);
        std::unique_ptr<tcp::socket> socket_ptr = std::make_unique<tcp::socket>(io_context);
        *socket_ptr = std::move(connection_pool.get_connection(local_endpoint));

        std::string message = "Hello, world!";
        boost::asio::write(*socket_ptr, boost::asio::buffer(message));

        char buffer[1024];
        size_t bytes_transferred = socket_ptr->read_some(boost::asio::buffer(buffer));
        std::cout << "Received " << bytes_transferred << " bytes: " << buffer << std::endl;

        connection_pool.return_connection(std::move(socket_ptr));
        socket_ptr.reset();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}