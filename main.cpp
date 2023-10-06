#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

int main() {
    boost::asio::io_context io_context;

    // Create endpoints
    std::vector<tcp::endpoint> endpoints;
    endpoints.emplace_back(boost::asio::ip::address::from_string("127.0.0.1"), 8000);
    endpoints.emplace_back(boost::asio::ip::address::from_string("127.0.0.1"), 8001);
    endpoints.emplace_back(boost::asio::ip::address::from_string("127.0.0.1"), 8002);

    // Create acceptors for each endpoint
    std::vector<tcp::acceptor> acceptors;
    acceptors.reserve(endpoints.size());
    for (const auto &endpoint: endpoints) {
        acceptors.emplace_back(io_context, endpoint);
    }
    while (true) {
        // Wait for a connection on any of the acceptors

        tcp::socket socket(io_context);
        boost::asio::ip::tcp::acceptor *acceptor = nullptr;
        for (auto &a: acceptors) {
            if (a.is_open()) {
                try {
                    a.accept(socket);
                    acceptor = &a;
                    break;
                } catch (const boost::system::system_error &e) {
                    // Ignore errors and try the next acceptor
                }
            }
        }

        if (acceptor) {
            char buffer[1024];
            size_t bytes_transferred = socket.read_some(boost::asio::buffer(buffer));
            std::cout << "Received " << bytes_transferred << " bytes: " << buffer << std::endl;

            std::string message = "Hello, client!";
            boost::asio::write(socket, boost::asio::buffer(message));

            socket.close();
        }
    }
}