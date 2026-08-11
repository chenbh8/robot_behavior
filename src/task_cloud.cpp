#include "task_cloud.hpp"

#include "logger.h"

namespace robot_behavior {

void TaskCloud::onStart() {
    LOG_INFO("task_cloud start");
    manager_->factory_.registerBehaviorTreeFromFile("../config/trees/sub_trees/navigation.xml");
    manager_->factory_.registerBehaviorTreeFromFile("../config/trees/sub_trees/get_box.xml");
    manager_->factory_.registerBehaviorTreeFromFile("../config/trees/sub_trees/put_box.xml");

    auto bb = BT::Blackboard::create();
    trees_["navigation"] = manager_->factory_.createTree("navigation", bb);
    trees_["get_box"] = manager_->factory_.createTree("get_box", bb);
    trees_["put_box"] = manager_->factory_.createTree("put_box", bb);
    std::string str = R"(
{
    "data": [
        [
            {
                "name": "navigation",
                "target": "room1"
            }
        ],
        [
            {
                "name": "get_box",
                "task_name": "wave"
            },
            {
                "name": "navigation",
                "target": "room2"
            }
        ],
        [
            {
                "name": "put_box"
            }
        ]
    ]
}
)";

    auto json = nlohmann::json::parse(str);
    parseTask(json);
}

void TaskCloud::tick() {
    // std::cout << "task_cloud tick, step_index: " << step_index_ << std::endl;
    if (step_index_ < 0 || step_index_ >= task_list_.size()) {
        return;
    }

    auto& group = task_list_[step_index_];

    // 1. tick当前组里面还没完成的action
    for (auto& action : group) {
        LOG_DEBUG("step_index: {}, task_cloud tick: {}, status: {}",
                      step_index_,
                      action.name,
                      static_cast<int>(action.status));
        if (action.status == BT::NodeStatus::SUCCESS) {
            continue;  // 已完成，不再tick
        }

        if (action.status == BT::NodeStatus::FAILURE) {
            return;
        }

        if (trees_.find(action.name) != trees_.end()) {
            auto& tree = trees_[action.name];
            if (!action.is_init) {
                tree.rootNode()->haltNode();
                // 初始化参数
                for (auto& [key, value] : action.params.items()) {
                    // std::cout << "step_index: " << step_index_ << ", key: " << key << ", value: "
                    // << value << std::endl;
                    if (key != "name")  // 排除name字段
                    {
                        if (value.is_string()) {
                            tree.rootBlackboard()->set(key, value.get<std::string>());
                        } else if (value.is_number_integer()) {
                            tree.rootBlackboard()->set(key, value.get<int>());
                        } else if (value.is_number_float()) {
                            tree.rootBlackboard()->set(key, value.get<double>());
                        } else if (value.is_boolean()) {
                            tree.rootBlackboard()->set(key, value.get<bool>());
                        }
                    }
                }

                action.is_init = true;
            }
            action.status = tree.rootNode()->executeTick();
        }
    }

    // 2. 当前组全部执行完后检查
    bool all_success = true;

    for (auto& action : group) {
        if (action.status != BT::NodeStatus::SUCCESS) {
            all_success = false;
            break;
        }
    }

    // 3. 当前并行组完成
    if (all_success) {
        step_index_++;

        if (step_index_ >= task_list_.size()) {
            return;
        }
    }

    return;
}

void TaskCloud::onStop() {
    LOG_INFO("task_cloud stop");
}

bool TaskCloud::parseTask(const nlohmann::json& json) {
    task_list_.clear();

    if (!json.contains("data") || !json["data"].is_array()) {
        return false;
    }

    for (const auto& group : json["data"]) {
        if (!group.is_array()) {
            return false;
        }

        std::vector<Action> actions;

        for (const auto& item : group) {
            if (!item.contains("name")) {
                return false;
            }

            Action action;

            action.name = item["name"].get<std::string>();

            action.status = BT::NodeStatus::IDLE;

            // 去掉name，剩余作为参数
            action.params = item;

            action.params.erase("name");

            actions.push_back(std::move(action));
        }

        task_list_.push_back(std::move(actions));
    }

    return true;
}

}  // namespace robot_behavior
