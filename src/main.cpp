#include "connection_pool.h"
#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

int main() {
    boost::asio::io_context io_context;
    tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 1234);
    ConnectionPool connection_pool(io_context, endpoint, 10);

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