#ifndef FLOWDB_RUNTIME_H
#define FLOWDB_RUNTIME_H


#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include "actor.h"

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; i++) {
            m_threads.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(m_mutex);
                        m_cv.wait(lock, [this] { return !m_tasks.empty() || m_done; });
                        if (m_done && m_tasks.empty()) {
                            return;
                        }
                        task = std::move(m_tasks.front());
                        m_tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_done = true;
        }
        m_cv.notify_all();
        for (auto &thread: m_threads) {
            thread.join();
        }
    }

    template<typename F>
    void submit(F &&f) {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_tasks.emplace(std::forward<F>(f));
        }
        m_cv.notify_one();
    }

private:
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_done = false;
};

class Runtime {
public:
    explicit Runtime(size_t num_threads) : m_thread_pool(num_threads), m_done(false) {}

    virtual ~Runtime() = default;

    // Template function to create an actor of type T
    template<typename T>
    std::shared_ptr<T> create_actor() {
        std::shared_ptr<T> actor = std::make_shared<T>();
        m_actors.push_back(actor);
        m_thread_pool.submit([actor] { actor->run(); });
        return actor;
    }

    // Stop all actors
    void stop() {
        for (const auto &actor: m_actors) {
            actor->stop();
        }
        m_done.store(true);
        m_cv.notify_one();
    }

    // Wait for all actors to finish executing
    void wait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return m_done.load(); });
    }

private:
    // Vector to hold all actors
    std::vector<std::shared_ptr<ActorBase>> m_actors;

    // Mutex to synchronize access to the actors vector
    std::mutex m_mutex;

    // Condition variable to signal when all actors have finished executing
    std::condition_variable m_cv;

    // Atomic boolean to indicate when all actors have finished executing
    std::atomic<bool> m_done;

    ThreadPool m_thread_pool;
};

#endif //FLOWDB_RUNTIME_H
