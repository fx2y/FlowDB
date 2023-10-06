#ifndef FLOWDB_RUNTIME_H
#define FLOWDB_RUNTIME_H


#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include "actor.h"

namespace flow {

    class Runtime {
    public:
        Runtime();

        virtual ~Runtime();
        
        // Template function to create an actor of type T
        template<typename T>
        std::shared_ptr<Actor<T>> create_actor();

        // Wait for all actors to finish executing
        void wait();

        // Stop all actors
        void stop();

    private:
        // Vector to hold all actors
        std::vector<std::shared_ptr<void>> m_actors;

        // Mutex to synchronize access to the actors vector
        std::mutex m_mutex;

        // Condition variable to signal when all actors have finished executing
        std::condition_variable m_cv;

        // Atomic boolean to indicate when all actors have finished executing
        std::atomic<bool> m_done;
    };

} // namespace flow

#include "runtime.cpp"

#endif //FLOWDB_RUNTIME_H
