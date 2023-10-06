#ifndef FLOWDB_ACTOR_H
#define FLOWDB_ACTOR_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>

namespace flow {

    template<typename T>
    class Actor {
    public:
        Actor();

        virtual ~Actor();

        // Sends a message to the actor.
        void tell(T msg);

        // Stops the actor and waits for it to finish processing messages.
        void stop();

        // Starts the actor's message processing loop.
        void run();

    protected:
        // Called when a message is received by the actor.
        // Subclasses should implement this method to handle messages.
        virtual void receive(T msg) = 0;

    private:
        // The queue of messages waiting to be processed by the actor.
        std::queue<T> m_queue;

        // The mutex used to synchronize access to the message queue.
        std::mutex m_mutex;

        // The condition variable used to signal when a new message is available.
        std::condition_variable m_cv;

        // A flag indicating whether the actor should stop processing messages.
        std::atomic<bool> m_done;
    };

} // namespace flow

#include "actor.cpp"

#endif //FLOWDB_ACTOR_H
