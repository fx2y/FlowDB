#include "runtime.h"
#include "actor.h"
#include "future.h"
#include <iostream>

class MyActor : public Actor<int> {
public:
    MyActor() : m_received(false) {}

    std::shared_ptr<Promise<int>> compute() {
        auto promise = std::make_shared<Promise<int>>();
        tell(promise, reinterpret_cast<void (Actor<int>::*)(std::shared_ptr<Promise<int>>&)>(&MyActor::handle_compute));
        return promise;
    }

    bool received() const {
        return m_received;
    }

protected:
    void receive(int msg) override {
        m_received = true;
    }

private:
    void handle_compute(const std::shared_ptr<Promise<int>>& promise) {
        int result = 42;
        promise->set_value(result);
    }

    bool m_received;
};

int main() {
    Runtime runtime(4);
    auto actor = runtime.create_actor<MyActor>();

    auto promise = actor->compute();

    int result = promise->get_future().get();

    std::cout << "Result: " << result << std::endl;
    std::cout << "Received: " << actor->received() << std::endl;

    actor->stop();
    runtime.stop();

    return 0;
}