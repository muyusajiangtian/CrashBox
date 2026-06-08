#pragma once
// AI控制系统 - 路径跟随、超车逻辑、撞击行为

#include "MathUtils.h"
#include "Vehicle.h"
#include "Terrain.h"
#include <cmath>
#include <vector>

namespace CrashBox {

// AI行为模式
enum class AIBehavior {
    PATH_FOLLOW,  // 路径跟随
    OVERTAKE,     // 超车
    RAM_PLAYER,   // 撞击玩家
    DEFEND        // 防守位置
};

// AI难度
enum class AIDifficulty {
    EASY,    // 简单：反应慢，少撞击
    MEDIUM,  // 中等：正常行为
    HARD     // 困难：积极撞击，精准走线
};

// AI控制器参数
struct AIParams {
    float maxThrottle = 0.85f;        // 最大油门比例
    float brakeDistance = 8.0f;        // 刹车提前距离
    float overtakeDistance = 3.0f;     // 超车触发距离
    float ramDistance = 5.0f;          // 撞击触发距离
    float ramCooldown = 5.0f;         // 撞击冷却时间
    float reactionDelay = 0.2f;       // 反应延迟(秒)
    float targetSpeedKmh = 60.0f;     // 目标速度
    float slopeSlowdown = 0.5f;       // 上坡减速系数
};

class AIController {
public:
    AIParams params;
    AIBehavior currentBehavior = AIBehavior::PATH_FOLLOW;
    AIDifficulty difficulty = AIDifficulty::MEDIUM;

    float ramCooldownTimer = 0.0f;
    float reactionTimer = 0.0f;
    float behaviorTimer = 0.0f;

    // 目标输出
    float outputThrottle = 0.0f;
    float outputBrake = 0.0f;
    float outputSteer = 0.0f;

    void setDifficulty(AIDifficulty diff) {
        difficulty = diff;
        switch (diff) {
            case AIDifficulty::EASY:
                params.maxThrottle = 0.6f;
                params.reactionDelay = 0.5f;
                params.ramCooldown = 15.0f;
                params.targetSpeedKmh = 40.0f;
                break;
            case AIDifficulty::MEDIUM:
                params.maxThrottle = 0.8f;
                params.reactionDelay = 0.2f;
                params.ramCooldown = 8.0f;
                params.targetSpeedKmh = 55.0f;
                break;
            case AIDifficulty::HARD:
                params.maxThrottle = 0.95f;
                params.reactionDelay = 0.05f;
                params.ramCooldown = 3.0f;
                params.targetSpeedKmh = 70.0f;
                break;
        }
    }

    // 主更新函数
    void update(const Vehicle& self, const Vehicle& player,
                const Terrain& terrain, float dt) {
        ramCooldownTimer = std::max(0.0f, ramCooldownTimer - dt);
        behaviorTimer += dt;

        Vec2 selfPos = self.chassis.getCenterOfMass();
        Vec2 playerPos = player.chassis.getCenterOfMass();
        float distToPlayer = (playerPos - selfPos).length();
        float selfX = selfPos.x;
        float playerX = playerPos.x;

        // 决策：选择行为
        decideBehavior(selfX, playerX, distToPlayer, self.speedKmh);

        // 执行行为
        switch (currentBehavior) {
            case AIBehavior::PATH_FOLLOW:
                executePathFollow(self, terrain, dt);
                break;
            case AIBehavior::OVERTAKE:
                executeOvertake(self, player, terrain, dt);
                break;
            case AIBehavior::RAM_PLAYER:
                executeRam(self, player, dt);
                break;
            case AIBehavior::DEFEND:
                executeDefend(self, player, terrain, dt);
                break;
        }
    }

    // 应用AI输出到车辆
    void applyToVehicle(Vehicle& vehicle) const {
        vehicle.throttleInput = outputThrottle;
        vehicle.brakeInput = outputBrake;
        vehicle.steerInput = outputSteer;
    }

private:
    void decideBehavior(float selfX, float playerX, float dist, float selfSpeed) {
        // 每2秒重新评估行为
        if (behaviorTimer < 2.0f && currentBehavior != AIBehavior::PATH_FOLLOW) return;
        behaviorTimer = 0.0f;

        bool playerAhead = (playerX > selfX);
        bool closeEnough = (dist < params.ramDistance);

        if (playerAhead && dist < params.overtakeDistance + 5.0f) {
            // 玩家在前方且距离近，尝试超车
            currentBehavior = AIBehavior::OVERTAKE;
        } else if (closeEnough && ramCooldownTimer <= 0.0f && !playerAhead) {
            // 玩家在后方靠近，主动撞击
            if (difficulty == AIDifficulty::HARD ||
                (difficulty == AIDifficulty::MEDIUM && dist < 3.0f)) {
                currentBehavior = AIBehavior::RAM_PLAYER;
            }
        } else if (!playerAhead && dist < 3.0f) {
            // 领先但被追上，防守
            currentBehavior = AIBehavior::DEFEND;
        } else {
            currentBehavior = AIBehavior::PATH_FOLLOW;
        }
    }

    // 路径跟随：根据前方地形调整油门和方向
    void executePathFollow(const Vehicle& self, const Terrain& terrain, float dt) {
        Vec2 pos = self.chassis.getCenterOfMass();
        float lookAhead = 5.0f + self.speedKmh * 0.1f;

        // 前方坡度检测
        float currentY = terrain.getHeightAt(pos.x);
        float aheadY = terrain.getHeightAt(pos.x + lookAhead);
        float slopeDiff = aheadY - currentY;

        // 速度控制
        if (self.speedKmh < params.targetSpeedKmh) {
            outputThrottle = params.maxThrottle;
            outputBrake = 0.0f;
        } else {
            outputThrottle = params.maxThrottle * 0.3f;
            outputBrake = 0.0f;
        }

        // 上坡提前加油
        if (slopeDiff < -1.0f) {
            outputThrottle = params.maxThrottle;
        }
        // 下坡或凹坑减速
        if (slopeDiff > 1.5f) {
            outputThrottle *= 0.5f;
            outputBrake = 0.3f;
        }

        outputSteer = 0.0f;
    }

    // 超车：加速并尝试绕过玩家
    void executeOvertake(const Vehicle& self, const Vehicle& player,
                         const Terrain& terrain, float dt) {
        outputThrottle = params.maxThrottle;
        outputBrake = 0.0f;

        Vec2 selfPos = self.chassis.getCenterOfMass();
        Vec2 playerPos = player.chassis.getCenterOfMass();

        // 如果已经超过玩家，回到路径跟随
        if (selfPos.x > playerPos.x + 2.0f) {
            currentBehavior = AIBehavior::PATH_FOLLOW;
            return;
        }

        // 全油门冲刺
        outputThrottle = 1.0f;
        outputSteer = 0.0f;
    }

    // 撞击：朝玩家方向施加力
    void executeRam(const Vehicle& self, const Vehicle& player, float dt) {
        Vec2 selfPos = self.chassis.getCenterOfMass();
        Vec2 playerPos = player.chassis.getCenterOfMass();
        Vec2 toPlayer = playerPos - selfPos;
        float dist = toPlayer.length();

        if (dist > params.ramDistance * 2.0f) {
            currentBehavior = AIBehavior::PATH_FOLLOW;
            return;
        }

        // 朝玩家冲刺
        outputThrottle = 1.0f;
        outputBrake = 0.0f;

        // 横向调整朝玩家
        if (toPlayer.x > 0.5f) {
            outputSteer = 1.0f;
        } else if (toPlayer.x < -0.5f) {
            outputSteer = -1.0f;
            outputThrottle = 0.3f; // 倒车撞
            outputBrake = 0.0f;
        }

        // 撞击完成后冷却
        if (dist < 1.5f) {
            ramCooldownTimer = params.ramCooldown;
            currentBehavior = AIBehavior::PATH_FOLLOW;
        }
    }

    // 防守：阻挡后方车辆超车
    void executeDefend(const Vehicle& self, const Vehicle& player,
                       const Terrain& terrain, float dt) {
        outputThrottle = params.maxThrottle * 0.9f;
        outputBrake = 0.0f;
        outputSteer = 0.0f;

        // 保持在前方
        Vec2 selfPos = self.chassis.getCenterOfMass();
        Vec2 playerPos = player.chassis.getCenterOfMass();

        if (selfPos.x < playerPos.x) {
            // 被超了，切回超车模式
            currentBehavior = AIBehavior::OVERTAKE;
        }
    }
};

} // namespace CrashBox
