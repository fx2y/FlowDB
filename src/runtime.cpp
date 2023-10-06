#include <thread>
#include "runtime.h"

namespace flow {
    
    // Constructor
    Runtime::Runtime() : m_done(false) {}

    // Destructor
    Runtime::~Runtime() = default;

    // Create a new actor of type T and start a new thread to run it
    template<typename T>
    std::shared_ptr<Actor<T>> Runtime::create_actor() {
        std::shared_ptr<Actor<T>> actor(new Actor<T>());
        m_actors.push_back(actor);
        std::thread t(&Actor<T>::run, actor.get());
        t.detach();
        return actor;
    }

    // Stop all actors and set a flag to indicate that the runtime is done
    void Runtime::stop() {
        for (const auto &actor: m_actors) {
            auto actor_ptr = static_cast<Actor<void *> *>(actor.get());
            actor_ptr->stop();
        }
        m_done.store(true);
        m_cv.notify_one();
    }

    // Wait for all actors to stop running
    void Runtime::wait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return m_done.load(); });
    }

} // namespace flow