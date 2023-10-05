#ifndef FLOWDB_CONNECTION_POOL_H
#define FLOWDB_CONNECTION_POOL_H

#include <boost/asio.hpp>
#include <deque>
#include <mutex>

using boost::asio::ip::tcp;

class ConnectionPool {
public:
    ConnectionPool(boost::asio::io_context &io_context, tcp::endpoint endpoint, int num_connections);

    tcp::socket& get_connection(const tcp::endpoint& local_endpoint);

    void return_connection(std::unique_ptr<tcp::socket> socket);

    void detect_failures();

private:
    boost::asio::io_context &io_context_;
    tcp::endpoint endpoint_;
    int num_connections_;
    int next_connection_;
    std::vector<std::unique_ptr<tcp::socket>> connections_;
    std::mutex mutex_;
};

#endif //FLOWDB_CONNECTION_POOL_H
