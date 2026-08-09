#include "robot_behavior.hpp"

#include <yaml-cpp/yaml.h>

#include <algorithm>

namespace robot_behavior {

bool BehaviorManager::loadConfig(const std::string& file) {
    auto root = YAML::LoadFile(file);

    for (auto item : root["behaviors"]) {
        BehaviorEntry entry;

        entry.id = item.first.as<std::string>();

        auto node = item.second;

        entry.priority = node["priority"].as<int>();

        if (node["request"]) {
            entry.request = node["request"].as<bool>();
        }

        if (node["behavior_path"]) {
            entry.behavior_path = node["behavior_path"].as<std::string>();
        }

        if (node["interrupt"]) {
            for (auto v : node["interrupt"]) {
                entry.interrupt.push_back(v.as<int>());
            }
        }

        if (node["pause"]) {
            for (auto v : node["pause"]) {
                entry.pause.push_back(v.as<int>());
            }
        }

        entries_.push_back(entry);
    }

    std::sort(entries_.begin(), entries_.end(), [](const auto& a, const auto& b) {
        return a.priority > b.priority;
    });

    return true;
}

void BehaviorManager::addBehavior(std::shared_ptr<Behavior> behavior) {
    auto entry = find(behavior->id());

    if (entry) {
        entry->behavior = behavior;
    }
}

void BehaviorManager::setRequest(const std::string& id, bool value) {
    auto entry = find(id);

    if (entry)
        entry->request = value;
}

void BehaviorManager::setEvent(const std::string& id, bool value) {
    auto entry = find(id);

    if (entry)
        entry->active = value;
}

BehaviorEntry* BehaviorManager::find(const std::string& id) {
    for (auto& entry : entries_) {
        if (entry.id == id)
            return &entry;
    }

    return nullptr;
}

bool BehaviorManager::contains(const std::vector<int>& list, int value) {
    return std::find(list.begin(), list.end(), value) != list.end();
}

void BehaviorManager::arbitration(BehaviorEntry& entry) {
    for (auto& other : entries_) {
        if (&entry == &other)
            continue;

        if (!other.behavior)
            continue;

        /*
         * 同等级不影响
         * 只能影响低等级
         */
        if (other.priority >= entry.priority)
            continue;

        if (contains(entry.interrupt, other.priority)) {
            other.allowed_start = false;

            if (other.behavior->state() == BehaviorState::RUNNING) {
                other.behavior->stop();
            }
        } else if (contains(entry.pause, other.priority)) {
            if (other.behavior->state() == BehaviorState::RUNNING) {
                other.behavior->pause();
            }
        }
    }
}

void BehaviorManager::tick() {
    /*
     * 1. 初始化候选状态
     */
    for (auto& entry : entries_) {
        if (!entry.behavior)
            continue;

        entry.allowed_start = entry.request;
    }

    /*
     * 2. 高优先级行为仲裁
     */
    for (auto& entry : entries_) {
        if (!entry.active)
            continue;

        arbitration(entry);
    }

    /*
     * 3. 提交状态
     */
    for (auto& entry : entries_) {
        if (!entry.behavior)
            continue;

        auto& behavior = entry.behavior;

        if (entry.allowed_start) {
            if (behavior->state() == BehaviorState::PAUSED) {
                behavior->resume();
            } else {
                behavior->start();
            }
        } else {
            if (behavior->state() == BehaviorState::RUNNING) {
                behavior->stop();
            }
        }
    }

    /*
     * 4. 执行
     */
    for (auto& entry : entries_) {
        if (!entry.behavior)
            continue;

        if (entry.behavior->state() == BehaviorState::RUNNING) {
            entry.behavior->tick();
        }
    }
}

}  // namespace robot_behavior