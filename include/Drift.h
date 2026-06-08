#pragma once
// 漂移机制 - 高速转弯时后轮侧向力超阈值进入漂移状态

#include "MathUtils.h"
#include <cmath>

namespace CrashBox {

// 漂移状态
struct DriftState {
    bool isDrifting = false;       // 是否正在漂移
    float driftAngle = 0.0f;      // 漂移角度
    float driftIntensity = 0.0f;  // 漂移强度 0~1
    float driftTimer = 0.0f;      // 漂移持续时间
    float lateralForce = 0.0f;    // 当前侧向力
};

// 漂移参数
struct DriftParams {
    float lateralThreshold = 3000.0f;  // 侧向力阈值，超过进入漂移
    float speedThreshold = 20.0f;      // 最低速度要求(km/h)
    float rearGripReduction = 0.4f;    // 漂移时后轮抓地力降低比例
    float counterSteerFactor = 1.5f;   // 反打方向盘效果加强
    float driftDecay = 2.0f;           // 漂移衰减速率
    float driftBuildUp = 5.0f;         // 漂移建立速率
    float maxDriftAngle = 45.0f;       // 最大漂移角度(度)
};

class DriftSystem {
public:
    DriftParams params;
    DriftState state;

    // 更新漂移状态
    void update(float speedKmh, float steerInput, float lateralForce,
                float frictionMul, float dt) {
        state.lateralForce = lateralForce;

        // 天气影响降低漂移阈值（湿滑路面更容易漂移）
        float effectiveThreshold = params.lateralThreshold * frictionMul;

        bool shouldDrift = (speedKmh > params.speedThreshold) &&
                           (std::abs(lateralForce) > effectiveThreshold) &&
                           (std::abs(steerInput) > 0.3f);

        if (shouldDrift && !state.isDrifting) {
            // 进入漂移
            state.isDrifting = true;
            state.driftTimer = 0.0f;
        }

        if (state.isDrifting) {
            state.driftTimer += dt;

            // 漂移强度建立
            state.driftIntensity += params.driftBuildUp * dt;
            state.driftIntensity = std::min(state.driftIntensity, 1.0f);

            // 漂移角度（基于转向输入和侧向力）
            float targetAngle = steerInput * params.maxDriftAngle * state.driftIntensity;
            state.driftAngle = lerp(state.driftAngle, targetAngle, dt * 3.0f);

            // 退出条件：速度过低、松开方向键
            if (speedKmh < params.speedThreshold * 0.6f ||
                std::abs(steerInput) < 0.1f) {
                state.isDrifting = false;
            }
        } else {
            // 漂移衰减
            state.driftIntensity -= params.driftDecay * dt;
            state.driftIntensity = std::max(state.driftIntensity, 0.0f);
            state.driftAngle *= (1.0f - dt * 5.0f);
        }
    }

    // 获取后轮抓地力修正
    float getRearGripMultiplier() const {
        if (state.isDrifting) {
            return 1.0f - params.rearGripReduction * state.driftIntensity;
        }
        return 1.0f;
    }

    // 获取额外横向力（漂移时产生侧滑）
    float getDriftLateralForce() const {
        if (state.isDrifting) {
            return state.driftAngle * 500.0f * state.driftIntensity;
        }
        return 0.0f;
    }
};

} // namespace CrashBox
