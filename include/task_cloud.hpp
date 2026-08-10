#pragma once
#include "robot_behavior.hpp"

namespace robot_behavior {
class TaskCloud : public Behavior {
public:
    TaskCloud(BehaviorManager* manager_) : Behavior("task_cloud"), manager_(manager_) {}

    void onStart() override;
    void tick() override;
    void onStop() override;

private:
    std::unordered_map<std::string, BT::Tree> trees_;
    BehaviorManager* manager_;
};
}  // namespace robot_behavior
