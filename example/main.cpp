#include "robot_behavior.hpp"
#include "task.hpp"
#include "task_cloud.hpp"
#include <iostream>
using namespace robot_behavior;

// class Navigation : public BT::SyncActionNode
class Navigation : public BT::StatefulActionNode {
public:
    Navigation(const std::string& name, const BT::NodeConfig& config)
        : BT::StatefulActionNode(name, config) {}

    static BT::PortsList providedPorts() {
        return {BT::InputPort<std::string>("target"), BT::OutputPort<std::string>("arm_error")};
    }
    BT::NodeStatus onStart() override {
        auto msg = getInput<std::string>("target");
        if (!msg) {
            throw BT::RuntimeError("missing required input [target]: ", msg.error());
        }
        std::cout << "Robot navigating to: " << msg.value() << std::endl;
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override {
        // 模拟导航过程
        static int counter = 0;
        counter++;
        if (counter >= 2)  // 假设导航需要5个tick完成
        {
            counter = 0;  // 重置计数器
            return BT::NodeStatus::SUCCESS;
        }
        // return BT::NodeStatus::FAILURE;
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override { std::cout << "Navigation halted!" << std::endl; }
};

class Motion : public BT::StatefulActionNode {
public:
    Motion(const std::string& name, const BT::NodeConfig& config)
        : BT::StatefulActionNode(name, config) {}

    static BT::PortsList providedPorts() {
        return {BT::InputPort<std::string>("task_name"),
                BT::InputPort<std::string>("yaml_args"),
                BT::OutputPort<bool>("status"),
                BT::OutputPort<std::string>("arm_error")};
    }
    BT::NodeStatus onStart() override {
        auto msg = getInput<std::string>("task_name");
        if (!msg) {
            throw BT::RuntimeError("missing required input [task_name]: ", msg.error());
        }
        std::cout << "Robot executing task: " << msg.value() << std::endl;
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override {
        // 模拟任务执行过程
        static int counter = 0;
        counter++;
        if (counter >= 3)  // 假设任务需要3个tick完成
        {
            counter = 0;  // 重置计数器
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override { std::cout << "Motion halted!" << std::endl; }
};

int main() {
    BehaviorManager manager;
    manager.loadConfig("../config/behaviors.yaml");
    manager.registerNode<Navigation>("Navigation");
    manager.registerNode<Motion>("Motion");
    manager.addBehavior(std::make_shared<Task>(&manager));
    manager.addBehavior(std::make_shared<TaskCloud>(&manager));
    while (true) {
        manager.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}