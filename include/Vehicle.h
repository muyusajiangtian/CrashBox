#pragma once
// 车辆模块 - 物理核心
// 修复清单：
//   1. 轮子接地点 = 轮毂中心在地面上方一个半径处（不是中心贴地）
//   2. 坡面重力分量：gravity * sin(slope) 沿坡面方向施加
//   3. 倒车加速：驱动力方向始终与chassisDir同向，不受当前速度方向限制
//   4. 前悬挂刚度加大，阻尼配比调高防止上坡塌底

#include "MathUtils.h"
#include "Collision.h"
#include "Tire.h"
#include "Suspension.h"
#include "Terrain.h"
#include <array>
#include <cmath>

namespace CrashBox {

struct VehicleConfig {
    float chassisWidth = 2.4f;
    float chassisHeight = 0.5f;
    float chassisMass = 800.0f;
    float wheelbase = 2.4f;
    float cgHeight = 0.3f;
    float engineForce = 5000.0f;
    float brakeForce = 8000.0f;
    float maxSteerAngle = 35.0f;
    float suspStiffnessFront = 65000.0f;  // 前悬挂刚度（更高抗塌底）
    float suspStiffnessRear = 50000.0f;   // 后悬挂刚度
    float suspDamping = 7000.0f;
    float suspRestLength = 0.55f;
    float suspBreakForce = 300000.0f;
    float springStiffness = 300000.0f;
    float springDamping = 3000.0f;
    float springBreakThreshold = 2.5f;
    float tireRadius = 0.35f;
    float rideHeight = 0.9f;
};

class Vehicle {
public:
    SoftBody chassis;
    std::array<Tire, 2> tires;             // [0]=前轮(右), [1]=后轮(左)
    std::array<Vec2, 2> wheelPositions;    // 轮毂中心位置
    std::array<int, 2> wheelAnchorIndices;
    std::array<bool, 2> wheelAttached = {true, true};
    std::array<bool, 2> suspBroken = {false, false};

    VehicleConfig config;

    float throttleInput = 0.0f;
    float brakeInput = 0.0f;
    float steerInput = 0.0f;

    float speed = 0.0f;
    float speedKmh = 0.0f;
    Vec2 gForce = {0, 0};
    Vec2 prevVelocity = {0, 0};
    float suspCompression[2] = {0.5f, 0.5f};

    void init(Vec2 startPos, const Terrain& terrain) {
        buildChassis(startPos, terrain);
        setupWheels(terrain);
    }

    void buildChassis(Vec2 center, const Terrain& terrain) {
        chassis.points.clear();
        chassis.springs.clear();

        float hw = config.chassisWidth / 2.0f;
        float hh = config.chassisHeight / 2.0f;
        float pointMass = config.chassisMass / 6.0f;

        float groundY = terrain.getHeightAt(center.x);
        // 底盘底部 = 地面 - rideHeight
        float bottomY = groundY - config.rideHeight;
        float centerY = bottomY - hh;

        Vec2 c = {center.x, centerY};

        chassis.addPoint(c + Vec2{-hw, -hh}, pointMass); // 0 左上
        chassis.addPoint(c + Vec2{  0, -hh}, pointMass); // 1 中上
        chassis.addPoint(c + Vec2{ hw, -hh}, pointMass); // 2 右上
        chassis.addPoint(c + Vec2{-hw,  hh}, pointMass); // 3 左下(后轮)
        chassis.addPoint(c + Vec2{  0,  hh}, pointMass); // 4 中下
        chassis.addPoint(c + Vec2{ hw,  hh}, pointMass); // 5 右下(前轮)

        float ks = config.springStiffness;
        float kd = config.springDamping;
        float bt = config.springBreakThreshold;

        chassis.addSpring(0, 1, ks, kd, bt);
        chassis.addSpring(1, 2, ks, kd, bt);
        chassis.addSpring(3, 4, ks, kd, bt);
        chassis.addSpring(4, 5, ks, kd, bt);
        chassis.addSpring(0, 3, ks, kd, bt);
        chassis.addSpring(2, 5, ks, kd, bt);
        chassis.addSpring(1, 4, ks, kd, bt);
        chassis.addSpring(0, 4, ks * 0.8f, kd, bt);
        chassis.addSpring(1, 3, ks * 0.8f, kd, bt);
        chassis.addSpring(1, 5, ks * 0.8f, kd, bt);
        chassis.addSpring(2, 4, ks * 0.8f, kd, bt);
        chassis.addSpring(0, 5, ks * 0.6f, kd, bt);
        chassis.addSpring(2, 3, ks * 0.6f, kd, bt);

        wheelAnchorIndices = {5, 3};
    }

    void setupWheels(const Terrain& terrain) {
        for (int i = 0; i < 2; ++i) {
            tires[i].radius = config.tireRadius;
            tires[i].inertia = 2.5f;
            Vec2 anchor = chassis.points[wheelAnchorIndices[i]].position;
            float groundY = terrain.getHeightAt(anchor.x);
            // 修复1：轮毂中心在地面上方一个轮胎半径
            wheelPositions[i] = {anchor.x, groundY - config.tireRadius};
            wheelAttached[i] = true;
            suspBroken[i] = false;
        }
    }

    void update(float dt, const Terrain& terrain) {
        chassis.clearForces();
        chassis.applyGravity(GRAVITY);

        Vec2 avgVel = chassis.getAverageVelocity();
        speed = std::abs(avgVel.x);
        speedKmh = speed * 3.6f;

        // 车身方向（前轮锚→后轮锚 = 从右到左在2D中，但我们定义前=右=正X）
        Vec2 frontAnchor = chassis.points[5].position;
        Vec2 rearAnchor = chassis.points[3].position;
        Vec2 chassisDir = (frontAnchor - rearAnchor).normalized();
        if (chassisDir.lengthSq() < 0.01f) chassisDir = {1, 0};

        // 重心转移
        float weightTransfer = 0.0f;
        if (brakeInput > 0.01f) {
            float decel = brakeInput * config.brakeForce / config.chassisMass;
            weightTransfer = config.chassisMass * decel * config.cgHeight / config.wheelbase;
        }

        float totalWeight = config.chassisMass * GRAVITY;
        float loads[2] = {
            totalWeight * 0.5f + weightTransfer,   // 前轮加载
            totalWeight * 0.5f - weightTransfer    // 后轮减载
        };

        // ===== 修复2：坡面重力分量 =====
        // 在每个轮子位置计算坡度，沿坡面方向施加重力分量到整车
        float avgSlopeAngle = 0.0f;
        int onGroundCount = 0;

        // ===== 处理每个轮子 =====
        for (int i = 0; i < 2; ++i) {
            if (!wheelAttached[i]) continue;

            int anchorIdx = wheelAnchorIndices[i];
            Vec2 anchorPos = chassis.points[anchorIdx].position;
            Vec2 anchorVel = chassis.points[anchorIdx].velocity;

            // 轮子X跟随锚点
            wheelPositions[i].x = anchorPos.x;

            // 地形高度（地面表面）
            float groundY = terrain.getHeightAt(wheelPositions[i].x);

            // 修复1：轮胎底部触地，轮毂中心 = 地面 - tireRadius
            float wheelBottomY = wheelPositions[i].y + config.tireRadius;
            bool onGround = (wheelBottomY >= groundY - 0.02f);

            if (onGround) {
                // 轮毂中心 = 地面高度 - 轮胎半径
                wheelPositions[i].y = groundY - config.tireRadius;
                onGroundCount++;

                // 记录坡度用于重力分量
                float slopeAngle = terrain.getSlopeAngleAt(wheelPositions[i].x);
                avgSlopeAngle += slopeAngle;
            } else {
                // 悬空时轮子下落
                wheelPositions[i].y += GRAVITY * dt;
                float targetY = groundY - config.tireRadius;
                if (wheelPositions[i].y > targetY) {
                    wheelPositions[i].y = targetY;
                    onGround = true;
                    onGroundCount++;
                    avgSlopeAngle += terrain.getSlopeAngleAt(wheelPositions[i].x);
                }
            }

            // --- 悬挂力 ---
            // 悬挂长度 = 锚点到轮毂中心的垂直距离
            float suspCurrentLen = wheelPositions[i].y - anchorPos.y;
            float compression = config.suspRestLength - suspCurrentLen;

            // 压缩比记录
            suspCompression[i] = clamp(
                (compression + 0.2f) / (config.suspRestLength + 0.2f), 0.0f, 1.0f);

            if (suspCurrentLen > 0.02f && onGround) {
                float compRate = anchorVel.y;

                // 修复4：前悬挂使用更高刚度
                float stiffness = (i == 0) ? config.suspStiffnessFront : config.suspStiffnessRear;
                float suspForce = stiffness * compression + config.suspDamping * compRate;

                // 限制拉伸力（悬挂只能推不能拉超过一定量）
                if (suspForce < -stiffness * 0.1f) {
                    suspForce = -stiffness * 0.1f;
                }

                if (std::abs(suspForce) > config.suspBreakForce) {
                    suspBroken[i] = true;
                    wheelAttached[i] = false;
                    continue;
                }

                // 悬挂力推底盘向上
                chassis.points[anchorIdx].force.y -= suspForce;
            }

            // --- 轮胎驱动力 ---
            if (onGround) {
                float normalLoad = std::max(100.0f, loads[i]);
                float vLong = avgVel.dot(chassisDir);

                // 修复3：驱动力矩始终正向（W=前进方向），不管当前速度方向
                float driveRatio = (i == 1) ? 0.7f : 0.3f;
                float driveTorque = throttleInput * config.engineForce * config.tireRadius * driveRatio;
                float brakeTorque = brakeInput * config.brakeForce * config.tireRadius * 0.5f;

                tires[i].computeForces(vLong, 0.0f, normalLoad, brakeTorque, driveTorque, dt);

                float fx = tires[i].state.longitudinalForce;
                fx = clamp(fx, -50000.0f, 50000.0f);
                Vec2 driveForce = chassisDir * fx;
                chassis.applyForce(anchorIdx, driveForce);
            }
        }

        // ===== 修复2：坡面重力分量施加到整车 =====
        if (onGroundCount > 0) {
            avgSlopeAngle /= onGroundCount;
            // sin(slopeAngle) 给出沿坡面的重力分量
            // 坡度角: 正值=上坡（Y减小方向=左高右低），对应坡面力向右(+X)
            float slopeForceX = config.chassisMass * GRAVITY * std::sin(avgSlopeAngle);
            // 分配到每个质点
            for (auto& p : chassis.points) {
                p.force.x += slopeForceX / (float)chassis.points.size();
            }
        }

        // ===== 左右控制 =====
        if (std::abs(steerInput) > 0.01f) {
            float lateralForce = steerInput * config.engineForce * 0.4f;
            for (auto& p : chassis.points) {
                p.force.x += lateralForce / (float)chassis.points.size();
            }
        }

        // ===== 底盘碰撞地形 =====
        for (auto& p : chassis.points) {
            float gY = terrain.getHeightAt(p.position.x);
            if (p.position.y > gY) {
                p.position.y = gY;
                if (p.velocity.y > 0) {
                    float impactV = p.velocity.y;
                    p.velocity.y = -impactV * 0.1f;
                    if (impactV > 5.0f) {
                        p.force.y -= impactV * 30000.0f;
                    }
                }
                p.velocity.x *= 0.95f;
            }
        }

        // ===== 底盘弹簧内力 =====
        chassis.computeSpringForces();

        // ===== 积分 =====
        chassis.integrate(dt);

        // 空气阻力（修复3：降低水平阻力让车能跑快、能倒车）
        for (auto& p : chassis.points) {
            p.velocity.x *= (1.0f - 0.3f * dt);  // 很小的阻力
            p.velocity.y *= (1.0f - 0.5f * dt);
        }

        // ===== G力 =====
        Vec2 currentVel = chassis.getAverageVelocity();
        if (dt > 0) {
            Vec2 accel = (currentVel - prevVelocity) / dt;
            gForce = gForce * 0.93f + (accel / GRAVITY) * 0.07f;
        }
        prevVelocity = currentVel;
    }

    bool isWheelDetached(int index) const { return !wheelAttached[index]; }

    float getDamageLevel() const {
        int total = static_cast<int>(chassis.springs.size()) + 2;
        int broken = chassis.brokenSpringCount();
        if (suspBroken[0]) ++broken;
        if (suspBroken[1]) ++broken;
        return static_cast<float>(broken) / static_cast<float>(total);
    }
};

} // namespace CrashBox
