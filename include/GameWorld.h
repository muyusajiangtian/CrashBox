#pragma once
// 游戏世界管理器 - 整合多车物理、AI、天气、碰撞、赛道

#include "MathUtils.h"
#include "Vehicle.h"
#include "Terrain.h"
#include "Weather.h"
#include "Drift.h"
#include "TowRope.h"
#include "VehicleCollision.h"
#include "AIController.h"
#include "RaceSystem.h"
#include "Config.h"
#include <SFML/Graphics/Color.hpp>
#include <vector>
#include <string>

namespace CrashBox {

// 多车实例（包含车辆+AI+漂移状态）
struct VehicleInstance {
    Vehicle vehicle;
    DriftSystem drift;
    AIController ai;
    bool isPlayer = false;
    bool isAI = false;
    int id = 0;
    std::string name;
    sf::Color color = sf::Color(200, 60, 60); // 车辆颜色
};

class GameWorld {
public:
    std::vector<VehicleInstance> vehicles;
    Terrain terrain;
    WeatherSystem weather;
    VehicleCollision collisionSystem;
    RaceSystem raceSystem;
    TowRope towRope;

    int playerVehicleId = 0;
    int maxVehicles = 4;

    // 初始化游戏世界
    void init(const Config& config) {
        // 生成赛道（更长的环形）
        std::string terrainType = config.getString("terrain_type", "preset");
        if (terrainType == "preset") {
            generateRaceTrack();
        } else {
            terrain.generateRandom(-5.0f,
                config.getFloat("terrain_length", 200.0f),
                0.0f,
                config.getFloat("terrain_segment_width", 3.0f),
                config.getFloat("terrain_max_slope", 0.3f));
        }

        // 初始化天气
        float trackLen = terrain.points.empty() ? 100.0f :
                         terrain.points.back().x - terrain.points.front().x;
        weather.init(trackLen);

        // 初始化赛道系统
        raceSystem.init(trackLen, 3);

        // 创建玩家车辆
        addPlayerVehicle(config);

        // 创建AI车辆
        addAIVehicle(config, "AI-1", AIDifficulty::MEDIUM,
                     sf::Color(60, 60, 200), 1);
        addAIVehicle(config, "AI-2", AIDifficulty::HARD,
                     sf::Color(60, 200, 60), 2);
        addAIVehicle(config, "AI-3", AIDifficulty::EASY,
                     sf::Color(200, 200, 60), 3);

        // 开始比赛
        raceSystem.startRace();
    }

    // 更新整个世界
    void update(float physicsDt, int substeps) {
        // 更新天气
        weather.update(physicsDt * substeps);

        // 更新赛道系统
        raceSystem.update(physicsDt * substeps);

        // 物理子步进
        for (int step = 0; step < substeps; ++step) {
            // AI决策（低频更新）
            if (step == 0) {
                updateAI(physicsDt * substeps);
            }

            // 更新每辆车的物理
            for (auto& vi : vehicles) {
                updateVehiclePhysics(vi, physicsDt);
            }

            // 车辆间碰撞检测和响应
            resolveAllCollisions();

            // 拖车绳力
            updateTowRope();
        }

        // 更新赛道位置和漂移
        for (auto& vi : vehicles) {
            Vec2 pos = vi.vehicle.chassis.getCenterOfMass();
            raceSystem.updateVehiclePosition(vi.id, pos.x, vi.vehicle.speedKmh);

            // 水坑溅水检测
            weather.checkPuddleSplash(pos.x, vi.vehicle.speed, vi.id);
        }
    }

    // 获取玩家车辆
    VehicleInstance* getPlayerVehicle() {
        for (auto& v : vehicles) {
            if (v.isPlayer) return &v;
        }
        return nullptr;
    }

    // 切换天气
    void cycleWeather() {
        WeatherType next;
        switch (weather.currentWeather) {
            case WeatherType::CLEAR: next = WeatherType::RAIN; break;
            case WeatherType::RAIN: next = WeatherType::ICE; break;
            case WeatherType::ICE: next = WeatherType::THAW; break;
            case WeatherType::THAW: next = WeatherType::CLEAR; break;
            default: next = WeatherType::CLEAR;
        }
        weather.setWeather(next);
    }

    // 连接/断开拖车绳
    void toggleTowRope(int vehicleA, int vehicleB) {
        if (towRope.state.connected) {
            towRope.disconnect();
        } else {
            towRope.connect(vehicleA, vehicleB);
        }
    }

    // 重置所有车辆
    void reset(const Config& config) {
        vehicles.clear();
        raceSystem = RaceSystem();

        float trackLen = terrain.points.empty() ? 100.0f :
                         terrain.points.back().x - terrain.points.front().x;
        raceSystem.init(trackLen, 3);

        addPlayerVehicle(config);
        addAIVehicle(config, "AI-1", AIDifficulty::MEDIUM,
                     sf::Color(60, 60, 200), 1);
        addAIVehicle(config, "AI-2", AIDifficulty::HARD,
                     sf::Color(60, 200, 60), 2);
        addAIVehicle(config, "AI-3", AIDifficulty::EASY,
                     sf::Color(200, 200, 60), 3);

        raceSystem.startRace();
        weather.setWeather(WeatherType::CLEAR);
        towRope.disconnect();
    }

private:
    // 生成比赛用赛道
    void generateRaceTrack() {
        terrain.points.clear();
        // 更长的赛道，有变化但不太极端
        terrain.points.push_back({-5.0f, 0.0f});
        terrain.points.push_back({15.0f, 0.0f});
        terrain.points.push_back({25.0f, -1.0f});
        terrain.points.push_back({35.0f, -1.5f});
        terrain.points.push_back({45.0f, -1.0f});
        terrain.points.push_back({55.0f, 0.5f});
        terrain.points.push_back({60.0f, 1.0f});
        terrain.points.push_back({65.0f, 0.5f});
        terrain.points.push_back({75.0f, 0.0f});
        terrain.points.push_back({85.0f, -2.0f});
        terrain.points.push_back({95.0f, -2.0f});
        terrain.points.push_back({105.0f, 0.0f});
        terrain.points.push_back({115.0f, 0.0f});
        terrain.points.push_back({125.0f, -1.0f});
        terrain.points.push_back({135.0f, 0.0f});
        terrain.points.push_back({150.0f, 0.0f});
        terrain.buildSegments();
    }

    void addPlayerVehicle(const Config& config) {
        VehicleInstance vi;
        vi.id = 0;
        vi.name = "玩家";
        vi.isPlayer = true;
        vi.isAI = false;
        vi.color = sf::Color(200, 60, 60);
        config.applyToVehicle(vi.vehicle.config);
        vi.vehicle.init({5.0f, 0.0f}, terrain);
        vehicles.push_back(vi);
        raceSystem.registerVehicle(0, "玩家");
    }

    void addAIVehicle(const Config& config, const std::string& name,
                      AIDifficulty diff, sf::Color color, int id) {
        VehicleInstance vi;
        vi.id = id;
        vi.name = name;
        vi.isPlayer = false;
        vi.isAI = true;
        vi.color = color;
        vi.ai.setDifficulty(diff);
        config.applyToVehicle(vi.vehicle.config);
        // AI车稍微错开起始位置
        float startX = 5.0f - id * 3.0f;
        vi.vehicle.init({startX, 0.0f}, terrain);
        vehicles.push_back(vi);
        raceSystem.registerVehicle(id, name);
    }

    void updateAI(float dt) {
        VehicleInstance* player = getPlayerVehicle();
        if (!player) return;

        for (auto& vi : vehicles) {
            if (!vi.isAI) continue;

            // 被溅水影响时AI反应变慢（短暂松油门）
            bool splashed = weather.isAffectedBySplash(
                vi.vehicle.chassis.getCenterOfMass().x, vi.id);

            vi.ai.update(vi.vehicle, player->vehicle, terrain, dt);

            if (splashed) {
                vi.ai.outputThrottle *= 0.5f;
            }

            vi.ai.applyToVehicle(vi.vehicle);
        }
    }

    void updateVehiclePhysics(VehicleInstance& vi, float dt) {
        // 天气影响轮胎力
        float frictionMul = weather.frictionMultiplier;
        float lateralMul = weather.lateralGripMultiplier;

        // 漂移系统更新
        float lateralF = vi.vehicle.steerInput * vi.vehicle.config.engineForce * 0.4f;
        vi.drift.update(vi.vehicle.speedKmh, vi.vehicle.steerInput,
                        lateralF, frictionMul, dt);

        // 暂存原始轮胎参数
        float origD0 = vi.vehicle.tires[0].longitudinalParams.D;
        float origD1 = vi.vehicle.tires[1].longitudinalParams.D;
        float origLD0 = vi.vehicle.tires[0].lateralParams.D;
        float origLD1 = vi.vehicle.tires[1].lateralParams.D;

        // 应用天气摩擦修正
        vi.vehicle.tires[0].longitudinalParams.D = origD0 * frictionMul;
        vi.vehicle.tires[1].longitudinalParams.D = origD1 * frictionMul;
        vi.vehicle.tires[0].lateralParams.D = origLD0 * lateralMul;
        vi.vehicle.tires[1].lateralParams.D = origLD1 * lateralMul;

        // 漂移时后轮抓地力降低
        float driftGrip = vi.drift.getRearGripMultiplier();
        vi.vehicle.tires[1].longitudinalParams.D *= driftGrip;
        vi.vehicle.tires[1].lateralParams.D *= driftGrip;

        // 车辆物理更新
        vi.vehicle.update(dt, terrain);

        // 漂移额外侧向力
        float driftForce = vi.drift.getDriftLateralForce();
        if (std::abs(driftForce) > 1.0f) {
            for (auto& p : vi.vehicle.chassis.points) {
                p.force.x += driftForce / (float)vi.vehicle.chassis.points.size();
            }
        }

        // 恢复原始轮胎参数
        vi.vehicle.tires[0].longitudinalParams.D = origD0;
        vi.vehicle.tires[1].longitudinalParams.D = origD1;
        vi.vehicle.tires[0].lateralParams.D = origLD0;
        vi.vehicle.tires[1].lateralParams.D = origLD1;
    }

    void resolveAllCollisions() {
        for (int i = 0; i < (int)vehicles.size(); ++i) {
            for (int j = i + 1; j < (int)vehicles.size(); ++j) {
                collisionSystem.resolveCollision(
                    vehicles[i].vehicle.chassis,
                    vehicles[j].vehicle.chassis,
                    vehicles[i].vehicle.config.chassisMass,
                    vehicles[j].vehicle.config.chassisMass
                );
            }
        }
    }

    void updateTowRope() {
        if (!towRope.state.connected) return;

        int idA = towRope.state.vehicleA;
        int idB = towRope.state.vehicleB;
        VehicleInstance* vA = nullptr;
        VehicleInstance* vB = nullptr;

        for (auto& v : vehicles) {
            if (v.id == idA) vA = &v;
            if (v.id == idB) vB = &v;
        }
        if (!vA || !vB) return;

        // 连接点：前车后端(质点3)，后车前端(质点5)
        Vec2 posA = vA->vehicle.chassis.points[3].position;
        Vec2 posB = vB->vehicle.chassis.points[5].position;
        Vec2 velA = vA->vehicle.chassis.points[3].velocity;
        Vec2 velB = vB->vehicle.chassis.points[5].velocity;

        Vec2 force = towRope.computeForce(posA, posB, velA, velB);

        if (force.lengthSq() > 0.01f) {
            // 由于Vehicle::update已经完成积分，这里用速度冲量方式施加约束
            // 等效于 force*dt/mass 作用在速度上
            float dt = 0.001f; // 与physicsDt一致
            float massA = vA->vehicle.config.chassisMass;
            float massB = vB->vehicle.config.chassisMass;
            int numPtsA = (int)vA->vehicle.chassis.points.size();
            int numPtsB = (int)vB->vehicle.chassis.points.size();

            // 后车被拉向前车
            Vec2 dvB = force * (dt / massB);
            for (auto& p : vB->vehicle.chassis.points) {
                p.velocity += dvB;
            }
            // 前车被拉住（牛顿第三）
            Vec2 dvA = force * (dt / massA);
            for (auto& p : vA->vehicle.chassis.points) {
                p.velocity -= dvA;
            }

            // 位置约束：绳索过长时直接缩短距离
            float dist = (posA - posB).length();
            if (dist > towRope.params.maxLength) {
                Vec2 dir = (posA - posB).normalized();
                float overStretch = dist - towRope.params.maxLength;
                float totalM = massA + massB;
                float moveB = overStretch * 0.5f * (massA / totalM);
                float moveA = overStretch * 0.5f * (massB / totalM);
                // 移动后车向前车
                for (auto& p : vB->vehicle.chassis.points) {
                    p.position += dir * (moveB / numPtsB);
                }
                // 移动前车向后车
                for (auto& p : vA->vehicle.chassis.points) {
                    p.position -= dir * (moveA / numPtsA);
                }
            }
        }
    }
};

} // namespace CrashBox
