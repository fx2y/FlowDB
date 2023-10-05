#include "connection_pool.h"

#include <utility>
#include <iostream>

/**
 * @brief Constructor for ConnectionPool class.
 * 
 * @param io_context The boost::asio::io_context object to use for asynchronous operations.
 * @param endpoint The TCP endpoint to connect to.
 */
ConnectionPool::ConnectionPool(boost::asio::io_context &io_context, tcp::endpoint endpoint, int num_connections)
        : io_context_(io_context), endpoint_(std::move(endpoint)), num_connections_(num_connections),
          next_connection_(0) {
    // Use the resize method to pre-allocate space for the connections vector
    connections_.resize(num_connections_);
    // Use a for loop to initialize each socket in the vector
    for (auto &socket: connections_) {
        socket = std::make_unique<tcp::socket>(io_context_);
    }
}

/**
 * @brief Returns a TCP socket from the connection pool.
 * 
 * If there is an available socket in the pool, it is returned. Otherwise, a new socket is created and returned.
 * 
 * @return std::shared_ptr<tcp::socket> A shared pointer to a TCP socket.
 */
// Use a const reference parameter to avoid unnecessary copying of endpoint objects
tcp::socket &ConnectionPool::get_connection(const tcp::endpoint &local_endpoint) {
    // Lock the mutex to ensure thread safety.
    std::unique_lock<std::mutex> lock(mutex_);
    // Use an assert to check that the connection pool is not empty
    assert(!connections_.empty());
    auto &socket = *connections_[next_connection_];
    next_connection_ = (next_connection_ + 1) % num_connections_;
    lock.unlock();
    // Check if the socket is already open
    if (!socket.is_open()) {
        // Open the socket if it is not already open
        socket.open(tcp::v4());
    }
    // Use the bind method to bind the socket to the local endpoint
    boost::system::error_code ec;
    socket.bind(local_endpoint, ec);
    if (ec) {
        // Handle the error if the bind method failed
        throw std::runtime_error("Failed to bind socket to local endpoint: " + ec.message());
    }
    // Use the connect method to connect the socket to the remote endpoint
    socket.connect(endpoint_);
    lock.lock();
    return socket;
}

/**
 * @brief Returns a TCP socket to the connection pool.
 *
 * @param socket A shared pointer to a TCP socket.
 */
// Use a rvalue reference parameter to allow move semantics
void ConnectionPool::return_connection(std::unique_ptr<tcp::socket> socket) {
    // Lock the mutex to ensure thread safety.
    std::unique_lock<std::mutex> lock(mutex_);
    if (!socket || !socket->is_open()) {
        // Handle the error if the socket is null or not open
        throw std::runtime_error("Socket is null or not open");
    }
    // Use the emplace_back method to move the socket to the back of the vector
    connections_.emplace_back(std::move(socket));
}

/**
 * Detects failures in the connections of the ConnectionPool object.
 * This function iterates over each socket in the connections vector, sends a "ping" message to the socket,
 * and checks if the socket fails to respond to the "ping" message. If a socket fails to respond,
 * the function closes and reconnects the socket to the endpoint.
 */
void ConnectionPool::detect_failures() {
    // Lock the mutex to prevent concurrent access to the connections vector
    std::unique_lock<std::mutex> lock(mutex_);

    // Iterate over each socket in the connections vector
    for (auto &socket: connections_) {
        // Set the socket to keep alive and non-blocking mode
        boost::system::error_code ec;
        socket->set_option(tcp::socket::keep_alive(true));
        socket->non_blocking(true);

        // Send a "ping" message to the socket
        socket->send(boost::asio::buffer("ping"), 0, ec);

        // If the socket fails to respond to the "ping" message
        if (ec) {
            // Close and reconnect the socket to the endpoint
            socket->shutdown(tcp::socket::shutdown_both, ec);
            socket->close(ec);
            socket->async_connect(endpoint_, [this, &socket](const boost::system::error_code &ec) {
                if (ec) {
                    // If the reconnection fails, close the socket
                    io_context_.post([this, &socket]() {
                        socket->close();
                        socket->connect(endpoint_);
                    });
                }
            });
        }

        // Set the socket back to blocking mode
        socket->non_blocking(false);
    }
}
