#include "task_cloud.hpp"

#include <iostream>

namespace robot_behavior {

void TaskCloud::onStart() {
    std::cout << "task_cloud start\n";
    manager_->factory_.registerBehaviorTreeFromFile("../config/trees/sub_trees/navigation.xml");
    manager_->factory_.registerBehaviorTreeFromFile("../config/trees/sub_trees/get_box.xml");
    manager_->factory_.registerBehaviorTreeFromFile("../config/trees/sub_trees/put_box.xml");

    auto bb = BT::Blackboard::create();
    trees_["navigation"] = manager_->factory_.createTree("navigation", bb);
    trees_["get_box"] = manager_->factory_.createTree("get_box", bb);
    trees_["put_box"] = manager_->factory_.createTree("put_box", bb);
}

void TaskCloud::tick() {
    std::cout << "task_cloud tick\n";
}

void TaskCloud::onStop() {
    std::cout << "task_cloud stop\n";
}

}  // namespace robot_behavior
