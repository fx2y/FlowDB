#include <uv.h>
#include <vector>

/**
 * @brief NetworkManager class that manages network connections using libuv library.
 */
class NetworkManager {
public:
    /**
     * @brief Constructor that initializes the server and sets its data to this instance.
     */
    NetworkManager() {
        uv_tcp_init(uv_default_loop(), &server_);
        server_.data = this;
    }

    /**
     * @brief Starts the server by binding it to the given host and port and listening for incoming connections.
     * 
     * @param host The host to bind the server to.
     * @param port The port to bind the server to.
     */
    void start(const char *host, int port) {
        struct sockaddr_in addr;
        uv_ip4_addr(host, port, &addr);

        uv_tcp_bind(&server_, (const struct sockaddr *) &addr, 0);
        uv_listen((uv_stream_t *) &server_, 10, on_new_connection);
    }

    /**
     * @brief Stops the server by closing all connections and the server handle.
     */
    void stop() {
        for (auto &conn: connections_) {
            uv_close((uv_handle_t *) &conn, on_connection_closed);
        }
        uv_close((uv_handle_t *) &server_, NULL);
    }

private:
    uv_tcp_t server_; /**< The server handle. */
    std::vector<uv_tcp_t> connections_; /**< The list of client connections. */

    /**
     * @brief Callback function for new incoming connections.
     * 
     * @param server The server handle.
     * @param status The status of the connection.
     */
    static void on_new_connection(uv_stream_t *server, int status) {
        NetworkManager *nm = (NetworkManager *) server->data;
        uv_tcp_t client;
        uv_tcp_init(uv_default_loop(), &client);
        uv_accept(server, (uv_stream_t *) &client);
        nm->connections_.push_back(client);
        uv_read_start((uv_stream_t *) &client, on_alloc_buffer, on_read_complete);
    }

    /**
     * @brief Callback function for connection closed event.
     * 
     * @param handle The handle of the connection that was closed.
     */
    static void on_connection_closed(uv_handle_t *handle) {
        NetworkManager *nm = (NetworkManager *) handle->data;
        auto &connections = nm->connections_;
        auto it = std::find_if(connections.begin(), connections.end(),
                               [&](const uv_tcp_t &conn) { return &conn == (uv_tcp_t *) handle; });
        if (it != connections.end()) {
            connections.erase(it);
        }
    }

    /**
     * @brief Callback function for read complete event.
     * 
     * @param stream The stream handle.
     * @param nread The number of bytes read.
     * @param buf The buffer containing the read data.
     */
    static void on_read_complete(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
        if (nread > 0) {
            uv_write_t *req = new uv_write_t;
            uv_buf_t response = uv_buf_init("OK", 2);
            uv_write(req, stream, &response, 1, on_write_complete);
        } else if (nread == UV_EOF) {
            uv_close((uv_handle_t *) stream, on_connection_closed);
        }
        delete[] buf->base;
    }

    /**
     * @brief Callback function for write complete event.
     * 
     * @param req The write request handle.
     * @param status The status of the write operation.
     */
    static void on_write_complete(uv_write_t *req, int status) {
        uv_close((uv_handle_t *) req->handle, on_connection_closed);
        delete req;
    }

    /**
     * @brief Callback function for buffer allocation event.
     * 
     * @param handle The handle of the stream.
     * @param suggested_size The suggested size of the buffer.
     * @param buf The buffer to allocate.
     */
    static void on_alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
        buf->base = new char[suggested_size];
        buf->len = suggested_size;
    }
};

int main() {
    NetworkManager nm;
    nm.start("0.0.0.0", 8080);
    // Wait for incoming connections
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    nm.stop();
    return 0;
}