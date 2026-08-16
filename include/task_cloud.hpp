#pragma once
#include "robot_behavior.hpp"

#include "logger.h"
struct Action {
    bool is_init = false;
    std::string name;
    BT::NodeStatus status{BT::NodeStatus::IDLE};
    nlohmann::json params;
};

namespace robot_behavior {
class TaskCloud : public Behavior {
public:
    TaskCloud(BehaviorManager* manager_) : Behavior("task_cloud"), manager_(manager_) {}

    void onStart() override;
    void tick() override;
    void onStop() override;
    bool parseTask(const nlohmann::json& json);

private:
    std::unordered_map<std::string, BT::Tree> trees_;
    BehaviorManager* manager_;
    std::vector<std::vector<Action>> task_list_;
    int step_index_ = 0;
};
}  // namespace robot_behavior
