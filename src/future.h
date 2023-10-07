#ifndef FLOWDB_FUTURE_H
#define FLOWDB_FUTURE_H

#include <mutex>
#include <condition_variable>
#include <future>

template<typename T>
class Promise;

template<typename T>
class Future {
public:
    Future() : m_promise(nullptr) {}

    Future(const Future &) = delete;

    // Move constructor
    Future(Future &&other) noexcept: m_promise(other.m_promise) {
        other.m_promise = nullptr;
    }

    ~Future() {
        if (m_promise) {
            m_promise->remove_future(this);
        }
    }

    Future &operator=(const Future &) = delete;

    // Move assignment operator
    Future &operator=(Future &&other) noexcept {
        if (this != &other) {
            if (m_promise) {
                m_promise->remove_future(this);
            }
            m_promise = other.m_promise;
            other.m_promise = nullptr;
        }
        return *this;
    }

    // Check if the future is valid
    [[nodiscard]] bool valid() const {
        return m_promise != nullptr;
    }

    // Get the value of the future
    T get() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return m_promise == nullptr || m_promise->has_value(); });
        if (m_promise) {
            return std::move(m_promise->get_value());
        } else {
            throw std::future_error(std::future_errc::no_state);
        }
    }

private:
    friend class Promise<T>;

    // Set the promise associated with this future
    void set_promise(Promise<T> *promise) {
        m_promise = promise;
    }

    // Reset the promise associated with this future
    void notify() {
        m_cv.notify_all();
    }

    // Pointer to the promise associated with this future
    Promise<T> *m_promise;

    // Mutex to protect access to the future
    std::mutex m_mutex;

    // Condition variable to wait for the value to be set
    std::condition_variable m_cv;
};

template<typename T>
class Promise {
public:
    Promise() : m_value_set(false) {}

    Promise(const Promise &) = delete;

    // Move constructor
    Promise(Promise &&other) noexcept: m_futures(std::move(other.m_futures)), m_value(std::move(other.m_value)),
                                       m_value_set(other.m_value_set.load()) {
        other.m_value_set = false;
    }

    ~Promise() {
        if (!m_value_set) {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_futures.clear();
        }
    }

    Promise &operator=(const Promise &) = delete;

    // Move assignment operator
    Promise &operator=(Promise &&other) noexcept {
        if (this != &other) {
            if (!m_value_set) {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_futures.clear();
            }
            m_futures = std::move(other.m_futures);
            m_value = std::move(other.m_value);
            m_value_set = other.m_value_set.load();
            other.m_value_set = false;
        }
        return *this;
    }

    // Get a future object associated with this promise
    Future<T> get_future() {
        Future<T> future;
        std::unique_lock<std::mutex> lock(m_mutex);
        future.set_promise(this);
        if (!m_value_set) {
            m_futures.push_back(&future);
        }
        return future;
    }

    // Set the value of the promise
    void set_value(T value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_value_set) {
            m_value = std::move(value);
            m_value_set = true;
            for (auto future: m_futures) {
                future->notify();
            }
            m_futures.clear();
        } else {
            throw std::future_error(std::future_errc::promise_already_satisfied);
        }
    }

    bool has_value() const {
        return m_value_set;
    }

    T &get_value() {
        return m_value;
    }

    void remove_future(Future<T> *future) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_futures.erase(std::remove(m_futures.begin(), m_futures.end(), future), m_futures.end());
    }

private:
    // List of associated futures
    std::vector<Future<T> *> m_futures;

    // The value of the promise
    T m_value;

    // Flag indicating whether the value has been set
    std::atomic<bool> m_value_set;

    // Mutex to protect access to the promise
    std::mutex m_mutex;
};

#endif //FLOWDB_FUTURE_H
