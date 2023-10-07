#ifndef FLOWDB_ACTOR_H
#define FLOWDB_ACTOR_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include "future.h"

class ActorBase {
public:
    virtual ~ActorBase() = default;

    virtual void stop() = 0;

    virtual void run() = 0;
};

template<typename T>
class Actor : public ActorBase {
public:
    Actor() : m_done(false) {}

    ~Actor() override = default;

    // Sends a message to the actor.
    void tell(std::shared_ptr<Promise<T>> &promise, void (Actor<T>::*method)(std::shared_ptr<Promise<T>> &)) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.emplace(method, &promise);
        m_cv.notify_one();
    }

    // Stops the actor and waits for it to finish processing messages.
    void stop() override {
        m_done = true;
        m_cv.notify_one();
    }

    // Starts the actor's message processing loop.
    void run() override {
        while (!m_done) {
            std::unique_lock<std::mutex> lock(m_mutex);

            // Wait until there is a message in the queue or the actor is stopped
            m_cv.wait(lock, [this] { return !m_queue.empty() || m_done; });
            while (!m_queue.empty()) {

                // Get the next message from the queue
                auto [method, promise] = std::move(m_queue.front());
                m_queue.pop();
                lock.unlock();

                // Process the message
                (this->*method)(*promise);
                lock.lock();
            }
        }
    }

protected:
    // Called when a message is received by the actor.
    // Subclasses should implement this method to handle messages.
    virtual void receive(T msg) = 0;

private:
    // The queue of messages waiting to be processed by the actor.
    std::queue<std::pair<void (Actor<T>::*)(std::shared_ptr<Promise<T>> &), std::shared_ptr<Promise<T>> *>> m_queue;

    // The mutex used to synchronize access to the message queue.
    std::mutex m_mutex;

    // The condition variable used to signal when a new message is available.
    std::condition_variable m_cv;

    // A flag indicating whether the actor should stop processing messages.
    std::atomic<bool> m_done;
};

#endif //FLOWDB_ACTOR_H
