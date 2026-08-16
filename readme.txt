# robot_behavior

基于优先级行为仲裁的机器人任务调度框架，集成 BehaviorTree.CPP 实现行为树驱动的任务编排。

## 概述

`robot_behavior` 提供了一套多行为并发仲裁机制：多个行为按优先级排序，高优先级行为可以**中断**或**暂停**低优先级行为。每个行为内部由 BehaviorTree 驱动执行，支持本地任务（Task）和云端任务（TaskCloud）两种模式。

### 核心流程

每个 tick 周期执行四个阶段：

```
┌──────────────────────────────────────────────────┐
│  1. 初始化候选状态                                 │
│     allowed_start = request                       │
├──────────────────────────────────────────────────┤
│  2. 高优先级行为仲裁                               │
│     active 的行为对低优先级行为执行 interrupt/pause │
├──────────────────────────────────────────────────┤
│  3. 提交状态                                       │
│     根据 allowed_start 决定 start/resume 或 stop   │
├──────────────────────────────────────────────────┤
│  4. 执行                                           │
│     RUNNING 状态的行为执行 tick                     │
└──────────────────────────────────────────────────┘
```

### 仲裁规则

- 同优先级行为互不影响
- 高优先级只能影响低优先级
- `interrupt` — 停止目标行为（行为回到 IDLE）
- `pause` — 暂停目标行为（行为保持 PAUSED，可恢复）

## 项目结构

```
robot_behavior/
├── CMakeLists.txt
├── config/
│   ├── behaviors.yaml                # 行为配置（优先级、中断/暂停关系）
│   └── trees/
│       ├── task.xml                  # Task 行为树定义
│       └── sub_trees/
│           ├── navigation.xml        # 导航子树
│           ├── get_box.xml           # 取货子树
│           └── put_box.xml           # 放货子树
├── include/
│   ├── logger.h                      # 日志封装（基于 spdlog）
│   ├── robot_behavior.hpp            # 核心类定义
│   ├── task.hpp                      # Task 本地任务
│   └── task_cloud.hpp               # TaskCloud 云端任务
├── src/
│   ├── robot_behavior.cpp            # BehaviorManager 实现
│   ├── task.cpp                      # Task 实现
│   └── task_cloud.cpp                # TaskCloud 实现
├── example/
│   └── main.cpp                      # 示例程序
├── third_party/
│   └── concurrentqueue/              # 无锁并发队列
├── format.sh                         # clang-format 脚本
└── .clang-format                     # 格式化配置
```

## 依赖

| 依赖 | 说明 |
|------|------|
| [BehaviorTree.CPP](https://github.com/BehaviorTree/BehaviorTree.CPP) | 行为树框架 |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp) | YAML 配置解析 |
| [spdlog](https://github.com/gabime/spdlog) | 异步日志 |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON 解析（TaskCloud 使用） |
| [concurrentqueue](https://github.com/cameron314/concurrentqueue) | 无锁并发队列（线程间任务派发） |

> spdlog 和 nlohmann/json 需系统安装；concurrentqueue 以 header-only 方式包含在 `third_party/`。

## 构建

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

构建产物：
- `librobot_behavior.so` — 共享库
- `demo` — 示例可执行程序

## 配置说明

### behaviors.yaml

```yaml
behaviors:
  emergency:                # 行为 ID
    priority: 100           # 优先级，数值越大越高
    request: false          # 初始是否请求运行
    interrupt:              # 可中断的优先级列表
      - 80
      - 50

  task:
    priority: 80
    request: false
    pause:                  # 可暂停的优先级列表
      - 50
    behavior_path: ../config/trees/task.xml   # 行为树 XML 路径

  task_cloud:
    priority: 50
    request: true           # 默认启动
```

### 行为树 XML

使用 BehaviorTree.CPP 的 XML 格式定义，支持子树引用：

```xml
<root BTCPP_format="4">
    <include path="sub_trees/navigation.xml"/>
    <BehaviorTree ID="MainTree">
        <Sequence name="root">
            <SubTree ID="navigation" target="{target}"/>
            <SubTree ID="get_box" task_name="get_box_task"/>
            <SubTree ID="put_box" task_name="put_box_task"/>
        </Sequence>
    </BehaviorTree>
</root>
```

## 使用示例

```cpp
#include "robot_behavior.hpp"
#include "task.hpp"
#include "task_cloud.hpp"
#include "logger.h"

// 1. 初始化日志
robot::Logger::init("logs/robot.log");

// 2. 创建管理器，加载配置
BehaviorManager manager;
manager.loadConfig("../config/behaviors.yaml");

// 3. 注册 BehaviorTree 节点类型
manager.registerNode<Navigation>("Navigation");
manager.registerNode<Motion>("Motion");

// 4. 添加行为实例
manager.addBehavior(std::make_shared<Task>(&manager));
manager.addBehavior(std::make_shared<TaskCloud>(&manager));

// 5. 启动主循环（1Hz tick）
manager.start();

// 6. 关闭日志
robot::Logger::shutdown();
```

### 线程间控制

外部线程可通过 `task_queue_` 异步控制行为：

```cpp
// 在其他线程中请求行为
manager.task_queue_.enqueue([&]() {
    manager.setRequest("task", true);
    manager.setEvent("task", true);
});
```

## 核心类说明

### Behavior

所有行为的基类，提供状态机管理：

```
IDLE ──start()──▶ RUNNING ──pause()──▶ PAUSED
  ▲                 │  ▲                  │
  │                 │  │                  │
  └───stop()────────┘  └──resume()───────┘
```

子类需实现：
- `onStart()` / `onStop()` — 启动/停止回调
- `onPause()` / `onResume()` — 暂停/恢复回调
- `tick()` — 周期执行逻辑

### BehaviorManager

核心管理器，负责：
- 从 YAML 加载行为配置
- 每周期执行仲裁和 tick
- 管理 BehaviorTree 工厂和节点注册
- 通过并发队列接收外部任务

### Task

本地行为任务，从 XML 文件加载一棵 BehaviorTree 并执行。

### TaskCloud

云端编排任务，支持：
- 从 JSON 解析多步骤并行任务
- 同一步骤内的行为并行 tick
- 步骤完成后自动推进
- 动态参数注入到 Blackboard

## 日志系统

基于 spdlog 的异步日志封装（`logger.h`）：

| 宏 | 级别 | 用途 |
|----|------|------|
| `LOG_DEBUG(...)` | Debug | 高频 tick、仲裁细节 |
| `LOG_INFO(...)` | Info | 生命周期事件、配置加载 |
| `LOG_WARN(...)` | Warn | 异常情况（未注册的 behavior） |
| `LOG_ERROR(...)` | Error | 错误 |
| `LOG_CRITICAL(...)` | Critical | 致命错误 |

特性：
- 异步写入，不阻塞主循环
- 同时输出到控制台（彩色）和旋转文件（50MB × 5 份）
- error 及以上级别立即刷盘
- 日志格式：`[2026-08-17 10:00:00.123] [thread 12345] [INFO] message`

## 代码格式化

```bash
./format.sh          # 格式化所有源文件
./format.sh check    # 仅检查格式
./format.sh diff     # 显示差异
```
