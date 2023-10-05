#include "connection_pool.h"

#include <utility>

/**
 * @brief Constructor for ConnectionPool class.
 * 
 * @param io_context The boost::asio::io_context object to use for asynchronous operations.
 * @param endpoint The TCP endpoint to connect to.
 */
ConnectionPool::ConnectionPool(boost::asio::io_context &io_context, tcp::endpoint endpoint, int num_connections)
        : io_context_(io_context), endpoint_(std::move(endpoint)), num_connections_(num_connections),
          next_connection_(0) {
    connections_.reserve(num_connections_);
    for (int i = 0; i < num_connections_; i++) {
        connections_.emplace_back(io_context_);
    }
}

/**
 * @brief Returns a TCP socket from the connection pool.
 * 
 * If there is an available socket in the pool, it is returned. Otherwise, a new socket is created and returned.
 * 
 * @return std::shared_ptr<tcp::socket> A shared pointer to a TCP socket.
 */
std::shared_ptr<tcp::socket> ConnectionPool::get_connection() {
    // Lock the mutex to ensure thread safety.
    std::unique_lock<std::mutex> lock(mutex_);
    if (connections_.empty()) {
        throw std::runtime_error("Connection pool is empty");
    }
    auto &socket = connections_[next_connection_];
    next_connection_ = (next_connection_ + 1) % num_connections_;
    return std::make_shared<tcp::socket>(std::move(socket));
}

/**
 * @brief Returns a TCP socket to the connection pool.
 *
 * @param socket A shared pointer to a TCP socket.
 */
void ConnectionPool::return_connection(const std::shared_ptr<tcp::socket>& socket) {
    // Lock the mutex to ensure thread safety.
    std::unique_lock<std::mutex> lock(mutex_);
    connections_.push_back(std::move(*socket));
}