#pragma once
#include "robot_behavior.hpp"

namespace robot_behavior {
class Task : public Behavior {
public:
    Task(BehaviorManager* manager_) : Behavior("task"), manager_(manager_) {}

    void onStart() override;
    void tick() override;
    void onStop() override;

private:
    BT::Tree tree_;
    BehaviorManager* manager_;
};
}  // namespace robot_behavior
