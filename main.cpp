#include <uv.h>
#include <vector>
#include <queue>
#include <unordered_set>
#include <random>

/**
 * @brief A class that manages network connections using libuv.
 */
class NetworkManager {
public:
    /**
     * @brief Constructs a NetworkManager object and initializes the server.
     */
    NetworkManager() {
        uv_tcp_init(uv_default_loop(), &server_);
        server_.data = this;
        connections_.reserve(max_pool_size_);
    }

    /**
     * @brief Starts the server and listens for incoming connections.
     * 
     * @param host The IP address or hostname to bind the server to.
     * @param port The port number to bind the server to.
     */
    void start(const char *host, int port) {
        struct sockaddr_in addr{};
        uv_ip4_addr(host, port, &addr);

        uv_tcp_bind(&server_, (const struct sockaddr *) &addr, 0);
        uv_listen((uv_stream_t *) &server_, 10, on_new_connection);
    }

    /**
     * @brief Stops the server and closes all active connections.
     */
    void stop() {
        for (auto &conn: connections_) {
            uv_close((uv_handle_t *) &conn, on_connection_closed);
        }
        uv_close((uv_handle_t *) &server_, nullptr);
    }

    /**
     * @brief Balances the number of connections in the connection pool.
     * 
     * If the number of active connections is less than the minimum pool size, new connections are added to the pool.
     * If the number of active connections is greater than the maximum pool size, excess connections are closed.
     */
    void balance_connections() {
        if (active_.size() < min_pool_size_) {
            int needed = min_pool_size_ - active_.size();
            for (int i = 0; i < needed; i++) {
                auto *conn = new uv_tcp_t;
                uv_tcp_init(uv_default_loop(), conn);
                pool_.push(conn);
            }
        } else if (active_.size() > max_pool_size_) {
            int excess = active_.size() - max_pool_size_;
            for (int i = 0; i < excess; i++) {
                uv_tcp_t &conn = get_random_connection();
                uv_close((uv_handle_t *) &conn, nullptr);
            }
        }
    }

    /**
     * @brief Detects failed connections and closes them.
     * 
     * If a connection has not been active for the specified failure detection interval, it is considered failed and closed.
     */
    void detect_failures() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_failure_).count();
        if (elapsed >= failure_detection_interval_) {
            for (auto &conn: connections_) {
                if (active_.count(conn) == 0) { // dereference conn to get a uv_tcp_t*
                    uv_close((uv_handle_t *) conn, nullptr);
                }
            }
            last_failure_ = now;
        }
    }

private:
    uv_tcp_t server_{};
    std::vector<uv_tcp_t *> connections_;
    std::queue<uv_tcp_t *> pool_;
    std::unordered_set<uv_tcp_t *> active_;
    std::mt19937 rng_{std::random_device{}()};
    std::chrono::steady_clock::time_point last_failure_{};

    static void on_new_connection(uv_stream_t *server, int status) {
        auto *nm = (NetworkManager *) server->data;
        uv_tcp_t *client = nm->get_connection(); // Get a connection from the pool
        accept_connection(nm, client); // Accept the connection
    }

    static void accept_connection(NetworkManager *nm, uv_tcp_t *client) {
        uv_accept((uv_stream_t *) &nm->server_, (uv_stream_t *) client); // Accept the connection
        nm->active_.insert(client); // Add the connection to the active set
        nm->connections_.push_back(client); // Add the connection to the connections vector
        uv_read_start((uv_stream_t *) client, on_alloc_buffer,
                      on_read_complete); // Start reading data from the connection
    }

    // This function is called when a connection is closed.
    static void on_connection_closed(uv_handle_t *handle) {
        auto *nm = (NetworkManager *) handle->data;
        auto it = nm->active_.find((uv_tcp_t *) handle);
        if (it != nm->active_.end()) {
            nm->active_.erase(it); // Remove the connection from the active set
            nm->pool_connection((uv_tcp_t *) handle); // Return the connection to the pool
            nm->connections_.erase(std::remove(nm->connections_.begin(), nm->connections_.end(), (uv_tcp_t *) handle),
                                   nm->connections_.end()); // Remove the connection from the connections vector
        }
    }

    // This function is called to allocate a buffer for reading data from a connection.
    static void on_alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
        buf->base = new char[suggested_size]; // Allocate a buffer for reading data from the connection
        buf->len = suggested_size;
    }

    // This function is called when data has been read from a connection.
    static void on_read_complete(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
        if (nread > 0) {
            // process the received data
        } else if (nread == UV_EOF) {
            // close the stream if the connection has been closed
            uv_close((uv_handle_t *) stream, on_connection_closed);
        }
        delete[] buf->base;
    }

    // This function returns a connection from the connection pool, or creates a new one if the pool is empty.
    uv_tcp_t *get_connection() {
        if (!pool_.empty()) {
            // return a connection from the pool if available
            uv_tcp_t *conn = pool_.front();
            pool_.pop();
            return conn;
        } else {
            // create a new connection if the pool is empty
            auto *conn = new uv_tcp_t;
            uv_tcp_init(uv_default_loop(), conn);
            return conn;
        }
    }

    // This function adds a connection to the connection pool, or closes it if the pool is full.
    void pool_connection(uv_tcp_t *conn) {
        if (pool_.size() < max_pool_size_) {
            // add the connection to the pool if there is space
            pool_.push(conn);
        } else {
            // close the connection if the pool is full
            uv_close((uv_handle_t *) conn, nullptr);
        }
    }

    // This function returns a random active connection.
    uv_tcp_t &get_random_connection() {
        if (connections_.empty()) {
            throw std::runtime_error("No connections available");
        }
        // generate a random index and return the corresponding connection
        std::uniform_int_distribution<int> dist(0, connections_.size() - 1);
        int index = dist(rng_);
        return *connections_[index];
    }

    // This function is called when a connection attempt is successful or fails.
    static void on_connect(uv_connect_t *req, int status) {
        if (status < 0) {
            // handle connection error
        } else {
            // handle connection success
        }
        delete req;
    }

    // This function is called when an IP address is resolved.
    static void on_resolve(uv_getaddrinfo_t *req, int status, struct addrinfo *res) {
        if (status < 0) {
            // handle resolve error
        } else {
            auto *conn = new uv_tcp_t;
            uv_tcp_init(uv_default_loop(), conn);
            auto *connect_req = new uv_connect_t;
            uv_tcp_connect(connect_req, conn, res->ai_addr, on_connect);
        }
        uv_freeaddrinfo(res);
        delete req;
    }


    int min_pool_size_ = 10;
    int max_pool_size_ = 100;
    int failure_detection_interval_ = 60;
};

int main() {
    // Create a NetworkManager instance.
    NetworkManager nm;

    // Start the NetworkManager instance with the IP address "127.0.0.1" and port 8080.
    nm.start("127.0.0.1", 8080);

    // Create a libuv event loop.
    uv_loop_t *loop = uv_default_loop();

    // Create a libuv timer.
    uv_timer_t timer;

    // Initialize the timer with the event loop.
    uv_timer_init(loop, &timer);

    // Start the timer with a callback function that runs every 1000 milliseconds.
    uv_timer_start(&timer, [](uv_timer_t *handle) {
        auto *nm = (NetworkManager *) handle->data;
        nm->balance_connections();
        nm->detect_failures();
    }, 0, 1000);

    // Start the event loop.
    uv_run(loop, UV_RUN_DEFAULT);

    // Stop the NetworkManager instance.
    nm.stop();

    return 0;
}