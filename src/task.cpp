#include "task.hpp"

#include "logger.h"

namespace robot_behavior {

void Task::onStart() {
    LOG_INFO("task start");
    BehaviorEntry* entry = manager_->find(id_);
    tree_ = manager_->factory_.createTreeFromFile(entry->behavior_path);
    tree_.rootBlackboard()->set("target", "target1");
}

void Task::tick() {
    LOG_DEBUG("task tick");
    tree_.rootNode()->executeTick();
}

void Task::onStop() {
    LOG_INFO("task stop");
}

}  // namespace robot_behavior
