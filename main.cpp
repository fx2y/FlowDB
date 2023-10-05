#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

int main() {
    boost::asio::io_context io_context;
    tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 1234);
    tcp::acceptor acceptor(io_context, endpoint);

    while (true) {
        tcp::socket socket(io_context);
        acceptor.accept(socket);

        char buffer[1024];
        size_t bytes_transferred = socket.read_some(boost::asio::buffer(buffer));
        std::cout << "Received " << bytes_transferred << " bytes: " << buffer << std::endl;

        std::string message = "Hello, client!";
        boost::asio::write(socket, boost::asio::buffer(message));

        socket.close();
    }

    return 0;
}