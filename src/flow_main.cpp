#include "runtime.h"
#include "actor.h"
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
    Runtime runtime;
    auto actor1 = runtime.create_actor<MyActor>();
    auto actor2 = runtime.create_actor<MyActor>();

    actor1->tell(actor2, new int(1));
    actor1->tell(actor2, new int(2));
    actor1->tell(actor2, new int(3));

    runtime.stop();

    std::cout << "Sum: " << actor1->get_sum() << std::endl;

    return 0;
}