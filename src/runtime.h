#ifndef FLOWDB_RUNTIME_H
#define FLOWDB_RUNTIME_H


#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include "actor.h"

class Runtime {
public:
    Runtime() : m_done(false) {}

    virtual ~Runtime() = default;

    // Template function to create an actor of type T
    template<typename T>
    std::shared_ptr<T> create_actor() {
        std::shared_ptr<T> actor = std::make_shared<T>();;
        m_actors.push_back(actor);
        std::thread t(&T::run, actor);
        t.detach();
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
};

#endif //FLOWDB_RUNTIME_H
