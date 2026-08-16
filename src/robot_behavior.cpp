#include "robot_behavior.hpp"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <csignal>

#include "logger.h"
namespace robot_behavior {

bool BehaviorManager::loadConfig(const std::string& file) {
    LOG_INFO("loading config from: {}", file);
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

        LOG_INFO("loaded behavior: id={}, priority={}, interrupt_count={}, pause_count={}",
                 entry.id,
                 entry.priority,
                 entry.interrupt.size(),
                 entry.pause.size());
        entries_.push_back(entry);
    }

    std::sort(entries_.begin(), entries_.end(), [](const auto& a, const auto& b) {
        return a.priority > b.priority;
    });

    LOG_INFO("config loaded, {} behaviors registered", entries_.size());
    return true;
}

std::atomic<bool> running{true};

void signalHandler(int signal) {
    if (signal == SIGINT) {
        running = false;
    }
}

void BehaviorManager::start() {
    auto next_time = std::chrono::steady_clock::now();

    while (running) {
        next_time += std::chrono::seconds(1);
        tick();
        std::this_thread::sleep_until(next_time);
    }
}

void BehaviorManager::addBehavior(std::shared_ptr<Behavior> behavior) {
    auto entry = find(behavior->id());

    if (entry) {
        entry->behavior = behavior;
        LOG_INFO("behavior added: {}", behavior->id());
    } else {
        LOG_WARN("behavior not found in config: {}", behavior->id());
    }
}

void BehaviorManager::setRequest(const std::string& id, bool value) {
    auto entry = find(id);

    if (entry) {
        entry->request = value;
        LOG_DEBUG("setRequest: id={}, value={}", id, value);
    } else {
        LOG_WARN("setRequest: unknown behavior id={}", id);
    }
}

void BehaviorManager::setEvent(const std::string& id, bool value) {
    auto entry = find(id);

    if (entry) {
        entry->active = value;
        LOG_DEBUG("setEvent: id={}, value={}", id, value);
    } else {
        LOG_WARN("setEvent: unknown behavior id={}", id);
    }
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
    LOG_DEBUG("arbitration: id={}, priority={}", entry.id, entry.priority);
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
                LOG_INFO("interrupt: {} stopped by {}", other.id, entry.id);
                other.behavior->stop();
            }
        } else if (contains(entry.pause, other.priority)) {
            if (other.behavior->state() == BehaviorState::RUNNING) {
                LOG_INFO("pause: {} paused by {}", other.id, entry.id);
                other.behavior->pause();
            }
        }
    }
}

void BehaviorManager::tick() {
    /*
     * 1. 初始化候选状态
     */
    processTask();
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
                LOG_DEBUG("resume: {}", entry.id);
                behavior->resume();
            } else {
                LOG_DEBUG("start: {}", entry.id);
                behavior->start();
            }
        } else {
            if (behavior->state() == BehaviorState::RUNNING) {
                LOG_DEBUG("stop: {}", entry.id);
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

void BehaviorManager::processTask() {
    std::function<void()> task;

    while (task_queue_.try_dequeue(task)) {
        task();
    }
}

}  // namespace robot_behavior