#ifndef FLOWDB_CONNECTION_POOL_H
#define FLOWDB_CONNECTION_POOL_H

#include <boost/asio.hpp>
#include <deque>
#include <mutex>

using boost::asio::ip::tcp;

class ConnectionPool {
public:
    ConnectionPool(boost::asio::io_context &io_context, tcp::endpoint endpoint);

    std::shared_ptr<tcp::socket> get_connection();

    void return_connection(const std::shared_ptr<tcp::socket>& socket);

private:
    void connect(const std::shared_ptr<tcp::socket>& socket);

    boost::asio::io_context &io_context_;
    tcp::endpoint endpoint_;
    std::deque<tcp::socket> connections_;
    std::mutex mutex_;
};

#endif //FLOWDB_CONNECTION_POOL_H