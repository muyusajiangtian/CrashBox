# CrashBox - 2D车辆物理模拟器（多车对抗版）

一个从零实现的2D车辆物理模拟器，使用C++和SFML 3.0构建。包含质点-弹簧软体底盘、Pacejka轮胎模型、弹簧阻尼悬挂系统、碰撞变形损坏机制、多车对抗、动态天气、漂移和赛道系统。

## 功能特性

### 核心物理
- **软体底盘**: 质点-弹簧网络构成可变形底盘，碰撞时真实变形
- **Pacejka轮胎模型**: 简化魔术公式计算纵向/侧向抓地力曲线
- **弹簧阻尼悬挂**: 连接底盘与车轮，模拟真实悬挂压缩回弹
- **重心转移**: 刹车时前轮载荷增加，体现真实制动力学
- **碰撞损坏**: 撞击超阈值断开弹簧，底盘变形严重时轮子脱落

### 多车对抗系统
- **4车同屏**: 1辆玩家车 + 3辆AI对手，每辆车独立物理模拟
- **车辆碰撞**: AABB宽相 + 质点对精确检测，正确的动量传递和碰撞变形
- **AI控制**: 路径跟随、超车逻辑、主动撞击玩家、防守位置
- **AI难度分级**: 简单/中等/困难三档，影响反应速度和攻击性

### 动态天气环境
- **雨天**: 轮胎摩擦系数降至60%，侧向抓地力降至50%，容易打滑侧滑
- **结冰**: 摩擦大幅下降至20%，极度湿滑
- **融冰过渡**: 从结冰状态渐变恢复，体现真实融冰过程
- **水坑溅水**: 经过水坑时高速产生溅水，遮挡后方车辆视线
- **天气自动轮换**: 30秒切换一次，含5秒渐变过渡

### 漂移机制
- **自动触发**: 高速转弯时后轮侧向力超过阈值自动进入漂移状态
- **湿滑加成**: 雨天/冰面降低漂移阈值，更容易触发漂移
- **后轮抓地降低**: 漂移时后轮抓地力动态降低
- **视觉效果**: 漂移时后轮产生烟雾粒子

### 拖车绳系统
- **弹性绳约束**: 车辆之间可连接拖车绳
- **张力计算**: 绳索超过自然长度才产生拉力（只拉不推）
- **断裂机制**: 张力超过阈值绳索断裂
- **颜色反馈**: 绳索颜色随张力变化（黄→橙→红）

### 赛道与排行榜
- **实时排行榜**: 显示各车位置、圈数、速度和与首位差距
- **检查点系统**: 赛道均匀分布检查点防止作弊
- **多圈比赛**: 默认3圈制

## 项目结构

```
CrashBox/
├── CMakeLists.txt              # CMake构建配置
├── config/
│   └── config.json             # 外置参数配置
├── include/
│   ├── MathUtils.h             # 数学工具(向量运算)
│   ├── Tire.h                  # 轮胎模块(Pacejka模型)
│   ├── Suspension.h            # 悬挂模块(弹簧阻尼)
│   ├── Terrain.h               # 地形模块(折线生成)
│   ├── Collision.h             # 碰撞损坏模块(软体系统)
│   ├── Vehicle.h               # 车辆模块(整合各系统)
│   ├── VehicleCollision.h      # 车辆间碰撞(动量传递+变形)
│   ├── AIController.h          # AI控制(路径跟随+超车+撞击)
│   ├── Weather.h               # 动态天气(雨/冰/融/水坑)
│   ├── Drift.h                 # 漂移机制(侧向力阈值触发)
│   ├── TowRope.h               # 拖车绳(弹性约束)
│   ├── RaceSystem.h            # 赛道排行榜(圈数+检查点)
│   ├── GameWorld.h             # 游戏世界管理器(整合所有系统)
│   ├── Renderer.h              # 渲染模块(SFML绘制)
│   ├── Config.h                # 配置模块(JSON解析)
│   └── Logger.h                # 日志模块
├── src/
│   └── main.cpp                # 主程序入口
└── tests/
    └── TESTING.md              # 测试文档
```

## 环境要求

- C++17 编译器 (GCC 13+ / MinGW-w64)
- CMake 3.16+
- SFML 3.0.1 (静态库版本)
- Windows 10/11

## 构建方法

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel 4
./CrashBox.exe
```

## 操作说明

| 按键 | 功能 |
|------|------|
| W / ↑ | 加速 |
| S / ↓ | 刹车 |
| A / ← | 左移 |
| D / → | 右移 |
| R | 重置所有车辆和比赛 |
| G | 切换天气(晴→雨→冰→融→晴) |
| T | 切换地形(重新生成随机地形) |
| C | 连接/断开拖车绳 |
| ESC | 退出 |

## 配置参数

编辑 `config/config.json` 修改参数，无需重新编译：

- `chassis_mass`: 底盘质量(kg)
- `engine_force`: 引擎最大驱动力(N)
- `brake_force`: 最大制动力(N)
- `suspension_stiffness_front/rear`: 前/后悬挂刚度(N/m)
- `suspension_break_force`: 悬挂断裂阈值(N)
- `spring_break_threshold`: 底盘弹簧断裂伸长倍数
- `terrain_type`: "preset"(预设赛道) 或 "random"(随机地形)
- `pixels_per_meter`: 渲染缩放比例

---

# CrashBox - 2D Vehicle Physics Simulator (Multi-Vehicle Combat Edition)

A 2D vehicle physics simulator built from scratch with C++ and SFML 3.0. Features mass-spring soft body chassis, Pacejka tire model, multi-vehicle combat, dynamic weather, drifting mechanics, and race system.

## Features

### Core Physics
- **Soft-body chassis**: Mass-spring network with realistic deformation
- **Pacejka tire model**: Simplified magic formula for grip curves
- **Spring-damper suspension**: Realistic compression and rebound
- **Collision damage**: Springs break on impact, wheels detach

### Multi-Vehicle Combat
- **4 vehicles on screen**: 1 player + 3 AI opponents with independent physics
- **Vehicle-to-vehicle collision**: AABB broad-phase + point-pair detection, momentum transfer and deformation
- **AI control**: Path following, overtaking, ramming, position defense
- **Difficulty levels**: Easy/Medium/Hard affecting reaction time and aggression

### Dynamic Weather
- **Rain**: Friction drops to 60%, lateral grip to 50%
- **Ice**: Friction drops to 20%, extremely slippery
- **Thaw transition**: Gradual recovery from ice state
- **Water puddles**: High-speed splashes obscure trailing vehicles

### Drift Mechanics
- **Auto-trigger**: Rear lateral force exceeds threshold during high-speed turns
- **Weather boost**: Lower threshold on wet/icy surfaces
- **Visual feedback**: Smoke particles during drift

### Tow Rope System
- **Elastic constraint**: Connect two vehicles with a rope
- **Tension-based**: Only pulls when stretched beyond rest length
- **Break mechanism**: Rope snaps if tension exceeds threshold

### Race & Leaderboard
- **Real-time leaderboard**: Position, lap count, speed, gap to leader
- **Checkpoint system**: Evenly distributed checkpoints
- **Multi-lap racing**: Default 3 laps

## Controls

| Key | Action |
|-----|--------|
| W / ↑ | Accelerate |
| S / ↓ | Brake |
| A / ← | Move left |
| D / → | Move right |
| R | Reset race |
| G | Cycle weather |
| T | Randomize terrain |
| C | Toggle tow rope |
| ESC | Quit |

## Build

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel 4
./CrashBox.exe
```

## License

MIT License
