#include "actor.h"

namespace flow {

    // a template class that implements an actor model
    template<typename T>
    Actor<T>::Actor() : m_done(false) {}

    template<typename T>
    Actor<T>::~Actor() = default;

    // adds a message to the actor's message queue
    template<typename T>
    void Actor<T>::tell(T msg) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(msg);
        m_cv.notify_one();
    }

    // sets a flag to stop the actor
    template<typename T>
    void Actor<T>::stop() {
        m_done = true;
        m_cv.notify_one();
    }

    // processes messages from the queue until the actor is stopped
    template<typename T>
    void Actor<T>::run() {
        while (!m_done) {
            std::unique_lock<std::mutex> lock(m_mutex);
            
            // Wait until there is a message in the queue or the actor is stopped
            m_cv.wait(lock, [this] { return !m_queue.empty() || m_done; });
            while (!m_queue.empty()) {

                // Get the next message from the queue
                T msg = m_queue.front();
                m_queue.pop();
                lock.unlock();

                // Process the message
                receive(msg);
                lock.lock();
            }
        }
    }

} // namespace flow