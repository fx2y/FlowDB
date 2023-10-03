#include "connection_pool.h"

#include <utility>

/**
 * @brief Constructor for ConnectionPool class.
 * 
 * @param io_context The boost::asio::io_context object to use for asynchronous operations.
 * @param endpoint The TCP endpoint to connect to.
 */
ConnectionPool::ConnectionPool(boost::asio::io_context &io_context, tcp::endpoint endpoint)
        : io_context_(io_context), endpoint_(std::move(endpoint)) {}

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
    if (!connections_.empty()) {
        auto socket = std::move(connections_.front());
        connections_.pop_front();
        return std::make_shared<tcp::socket>(std::move(socket));
    } else {
        auto socket = std::make_shared<tcp::socket>(io_context_);
        connect(socket);
        return socket;
    }
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

/**
 * @brief Establishes a connection to the endpoint and adds the socket to the connection pool.
 * 
 * @details Asynchronously connects to the endpoint using the specified socket. If the connection is successful,
 * the socket is added to the connection pool. If the connection fails, the function is called recursively to
 * attempt to connect again.
 *
 * @param socket A shared pointer to a TCP socket.
 */
void ConnectionPool::connect(const std::shared_ptr<tcp::socket>& socket) {
    // Asynchronously connect to the endpoint.
    socket->async_connect(endpoint_, [this, socket](const boost::system::error_code &error) {
        // If the connection is successful, add the socket to the connection pool.
        if (!error) {
            return_connection(socket);
        }
            // If the connection fails, call the function recursively to attempt to connect again.
        else {
            connect(socket);
        }
    });
}