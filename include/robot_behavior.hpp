#pragma once
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "behaviortree_cpp/bt_factory.h"
using namespace BT;
namespace robot_behavior {

enum class BehaviorState { IDLE, RUNNING, PAUSED };

class Behavior {
public:
    explicit Behavior(const std::string& id) : id_(id) {}

    virtual ~Behavior() = default;

    const std::string& id() const { return id_; }

    virtual void onStart() {}
    virtual void onStop() {}
    virtual void onPause() {}
    virtual void onResume() {}
    virtual void tick() {}

    void start() {
        if (state_ == BehaviorState::RUNNING)
            return;

        onStart();
        state_ = BehaviorState::RUNNING;
    }

    void stop() {
        if (state_ == BehaviorState::IDLE)
            return;

        onStop();
        state_ = BehaviorState::IDLE;
    }

    void pause() {
        if (state_ != BehaviorState::RUNNING)
            return;

        onPause();
        state_ = BehaviorState::PAUSED;
    }

    void resume() {
        if (state_ != BehaviorState::PAUSED)
            return;

        onResume();
        state_ = BehaviorState::RUNNING;
    }

    BehaviorState state() const { return state_; }

public:
    std::string id_;

private:
    BehaviorState state_{BehaviorState::IDLE};
};

struct BehaviorEntry {
    std::string id;

    int priority{0};

    // 是否请求运行
    bool request{false};

    // 外部事件状态
    bool active{false};

    // 当前tick仲裁结果
    bool allowed_start{false};

    std::string behavior_path;
    // 可以停止哪些等级
    std::vector<int> interrupt;

    // 可以暂停哪些等级
    std::vector<int> pause;

    std::shared_ptr<Behavior> behavior;
};

class BehaviorManager {
public:
    bool loadConfig(const std::string& file);

    void addBehavior(std::shared_ptr<Behavior> behavior);

    void setRequest(const std::string& id, bool value);

    void setEvent(const std::string& id, bool value);

    void tick();
    BehaviorEntry* find(const std::string& id);
    template <typename T>
    void registerNode(const std::string& name) {
        factory_.registerNodeType<T>(name);
    }

private:
    bool contains(const std::vector<int>& list, int value);

    void arbitration(BehaviorEntry& entry);

private:
    std::vector<BehaviorEntry> entries_;

public:
    BehaviorTreeFactory factory_;
};

}  // namespace robot_behavior