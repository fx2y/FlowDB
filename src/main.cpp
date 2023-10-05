#include "connection_pool.h"
#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred) {
    if (!error) {
        std::cout << "Data sent successfully" << std::endl;
    } else {
        std::cerr << "Error sending data: " << error.message() << std::endl;
    }
}

int main() {
    boost::asio::io_context io_context;
    tcp::endpoint endpoint{tcp::v4(), 12345};
    ConnectionPool connection_pool(io_context, endpoint, 5);

    // Obtain a socket from the connection pool
    std::shared_ptr<tcp::socket> socket_ptr = connection_pool.get_connection();

    if (socket_ptr->is_open()) {
        // Use the socket to communicate with other fdbserver processes
        boost::asio::async_write(*socket_ptr, boost::asio::buffer("Hello, world!"), handle_write);
    }

    // Return the socket to the connection pool
    connection_pool.return_connection(socket_ptr);

    io_context.run();

    return 0;
}