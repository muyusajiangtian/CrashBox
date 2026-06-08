# CrashBox - 2D车辆物理模拟器

一个从零实现的2D车辆物理模拟器，使用C++和SFML 3.0构建。包含质点-弹簧软体底盘、Pacejka轮胎模型、弹簧阻尼悬挂系统、碰撞变形损坏机制和折线段地形。

## 功能特性

- **软体底盘**: 质点-弹簧网络构成可变形底盘，碰撞时真实变形
- **Pacejka轮胎模型**: 简化魔术公式计算纵向/侧向抓地力曲线
- **弹簧阻尼悬挂**: 连接底盘与车轮，模拟真实悬挂压缩回弹
- **重心转移**: 刹车时前轮载荷增加，体现真实制动力学
- **碰撞损坏**: 撞击超阈值断开弹簧，底盘变形严重时轮子脱落
- **折线段地形**: 支持坡度起伏，车辆在坡上正确受重力分量影响
- **随机地形/预设赛道**: T键切换，支持多种地形体验
- **实时HUD**: 显示速度、G力、悬挂压缩量、损坏程度
- **外置配置**: JSON文件配置所有物理参数，无需重新编译

## 项目结构

```
CrashBox/
├── CMakeLists.txt          # CMake构建配置
├── run.sh                  # 一键构建运行脚本
├── config/
│   └── config.json         # 外置参数配置
├── include/
│   ├── MathUtils.h         # 数学工具(向量运算)
│   ├── Tire.h              # 轮胎模块(Pacejka模型)
│   ├── Suspension.h        # 悬挂模块(弹簧阻尼)
│   ├── Terrain.h           # 地形模块(折线生成)
│   ├── Collision.h         # 碰撞损坏模块(软体系统)
│   ├── Vehicle.h           # 车辆模块(整合各系统)
│   ├── Renderer.h          # 渲染模块(SFML绘制)
│   ├── Config.h            # 配置模块(JSON解析)
│   └── Logger.h            # 日志模块
├── src/
│   └── main.cpp            # 主程序入口
└── docs/
    └── TESTING.md          # 手动测试文档
```

## 环境要求

- C++17 编译器 (GCC 13+ / MinGW-w64)
- CMake 3.16+
- SFML 3.0.1 (静态库版本)
- Windows 10/11

## 构建方法

### 方法一：使用脚本（推荐）

```bash
# 构建并运行
./run.sh

# 仅构建
./run.sh build

# 仅运行
./run.sh run

# 清理
./run.sh clean
```

### 方法二：手动CMake

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DSFML_DIR="E:/SFML/SFML-3.0.1-gcc15/lib/cmake/SFML"
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
| R | 重置车辆 |
| T | 切换地形(预设/随机) |
| ESC | 退出 |

## 配置参数

编辑 `config/config.json` 修改参数，无需重新编译：

- `chassis_mass`: 底盘质量(kg)
- `engine_force`: 引擎最大驱动力(N)
- `brake_force`: 最大制动力(N)
- `suspension_stiffness`: 悬挂弹簧刚度(N/m)
- `suspension_break_force`: 悬挂断裂阈值(N)
- `spring_break_threshold`: 底盘弹簧断裂伸长倍数
- `terrain_type`: "preset"(预设赛道) 或 "random"(随机地形)
- `pixels_per_meter`: 渲染缩放比例

## 物理模型说明

### Pacejka轮胎模型
使用简化魔术公式: `F = D × sin(C × atan(B×x - E×(B×x - atan(B×x))))`
- 纵向力根据滑移率计算
- 侧向力根据侧偏角计算
- 联合滑移用椭圆模型约束

### 重心转移
刹车时: `ΔLoad = m × a × h / L`
- m: 车辆质量
- a: 减速度
- h: 重心高度
- L: 轴距

### 损坏系统
弹簧伸长超过自然长度 × breakThreshold 倍时断裂，表现为底盘变形。悬挂力超过breakForce时断裂，轮子脱落。

---

# CrashBox - 2D Vehicle Physics Simulator (English)

A 2D vehicle physics simulator built from scratch with C++ and SFML 3.0. Features mass-spring soft body chassis, Pacejka tire model, spring-damper suspension, collision deformation, and polyline terrain.

## Features

- **Soft-body chassis**: Mass-spring network with realistic deformation on impact
- **Pacejka tire model**: Simplified magic formula for longitudinal/lateral grip curves
- **Spring-damper suspension**: Connecting chassis to wheels with compression/rebound
- **Weight transfer**: Front wheel load increases under braking
- **Collision damage**: Springs break on impact, wheels detach on severe deformation
- **Polyline terrain**: Slopes and bumps with correct gravity decomposition
- **Random/preset terrain**: Toggle with T key
- **Real-time HUD**: Speed, G-force, suspension compression, damage level

## Build Instructions

### Requirements
- C++17 compiler (GCC 13+ / MinGW-w64)
- CMake 3.16+
- SFML 3.0.1 (static libraries)
- Windows 10/11

### Quick Build
```bash
./run.sh        # Build and run
./run.sh build  # Build only
./run.sh clean  # Clean build directory
```

### Manual Build
```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DSFML_DIR="/path/to/SFML/lib/cmake/SFML"
cmake --build . --parallel 4
./CrashBox.exe
```

## Controls

| Key | Action |
|-----|--------|
| W / ↑ | Accelerate |
| S / ↓ | Brake |
| A / ← | Move left |
| D / → | Move right |
| R | Reset vehicle |
| T | Toggle terrain |
| ESC | Quit |

## License

MIT License
