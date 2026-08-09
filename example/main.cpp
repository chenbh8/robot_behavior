#include "robot_behavior.hpp"

#include <iostream>

using namespace robot_behavior;

class Navigation : public Behavior {
public:
    Navigation() : Behavior("navigation") {}

    void onStart() override { std::cout << "navigation start\n"; }

    void tick() override { std::cout << "navigation tick\n"; }
};

int main() {
    BehaviorManager manager;

    manager.loadConfig("behaviors.yaml");

    manager.addBehavior(std::make_shared<Navigation>());

    while (true) {
        manager.tick();
    }
}