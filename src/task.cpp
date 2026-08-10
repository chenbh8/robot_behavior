#include "task.hpp"

#include <iostream>

namespace robot_behavior {

void Task::onStart() {
    std::cout << "task start\n";
    BehaviorEntry* entry = manager_->find(id_);
    tree_ = manager_->factory_.createTreeFromFile(entry->behavior_path);
    tree_.rootBlackboard()->set("target", "target1");
}

void Task::tick() {
    std::cout << "task tick\n";
    tree_.rootNode()->executeTick();
}

void Task::onStop() {
    std::cout << "task stop\n";
}

}  // namespace robot_behavior
