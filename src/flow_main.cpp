#include "runtime.h"
#include "actor.h"
#include "future.h"
#include <iostream>

class MyActor : public Actor<int> {
public:
    MyActor() : m_sum(0) {}

    void receive(std::shared_ptr<Actor<int>> sender, int msg) override {
        m_sum += msg;
    }

    int get_sum() const {
        return m_sum;
    }

private:
    int m_sum;
};

int main() {
    Promise<int> promise;
    Future<int> future = promise.get_future();

    std::thread t([&promise] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        promise.set_value(42);
    });

    int result = future.get();

    t.join();

    std::cout << "Result: " << result << std::endl;

    return 0;
}