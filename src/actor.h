#ifndef FLOWDB_ACTOR_H
#define FLOWDB_ACTOR_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>

class ActorBase {
public:
    virtual ~ActorBase() = default;

    virtual void tell(std::shared_ptr<ActorBase> sender, void *msg) = 0;

    virtual void stop() = 0;

    virtual void run() = 0;
};

template<typename T>
class Message;

template<typename T>
class Actor : public ActorBase {
public:
    Actor() : m_done(false) {}

    ~Actor() override = default;

    // Sends a message to the actor.
    void tell(std::shared_ptr<ActorBase> sender, void *msg) override {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(Message<T>(std::static_pointer_cast<Actor<T>>(sender), *static_cast<T *>(msg)));
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
                Message<T> msg = m_queue.front();
                m_queue.pop();
                lock.unlock();

                // Process the message
                receive(msg.sender, msg.data);
                lock.lock();
            }
        }
    }

protected:
    // Called when a message is received by the actor.
    // Subclasses should implement this method to handle messages.
    virtual void receive(std::shared_ptr<Actor<T>> sender, T msg) = 0;

private:
    // The queue of messages waiting to be processed by the actor.
    std::queue<Message<T>> m_queue;

    // The mutex used to synchronize access to the message queue.
    std::mutex m_mutex;

    // The condition variable used to signal when a new message is available.
    std::condition_variable m_cv;

    // A flag indicating whether the actor should stop processing messages.
    std::atomic<bool> m_done;
};

template<typename T>
class Message {
public:
    Message(std::shared_ptr<Actor<T>> sender, T data) : sender(sender), data(data) {}

    std::shared_ptr<Actor<T>> sender;
    T data;
};

#endif //FLOWDB_ACTOR_H
