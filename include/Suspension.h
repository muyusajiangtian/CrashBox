#pragma once
// 悬挂模块 - 弹簧阻尼系统连接底盘与车轮

#include "MathUtils.h"

namespace CrashBox {

struct SuspensionParams {
    float stiffness = 30000.0f;    // 弹簧刚度 (N/m)
    float damping = 3000.0f;       // 阻尼系数 (N·s/m)
    float restLength = 0.4f;       // 自然长度 (m)
    float maxCompression = 0.3f;   // 最大压缩量 (m)
    float maxExtension = 0.15f;    // 最大伸展量 (m)
    float breakForce = 80000.0f;   // 断裂力阈值 (N)
};

struct SuspensionState {
    float compression = 0.0f;      // 当前压缩量 (m)
    float compressionRatio = 0.0f; // 压缩比 (0~1)
    float force = 0.0f;            // 当前悬挂力 (N)
    float velocity = 0.0f;         // 压缩速度 (m/s)
    bool broken = false;           // 是否断裂
};

class Suspension {
public:
    SuspensionParams params;
    SuspensionState state;
    Vec2 chassisAnchor;   // 底盘连接点（局部坐标）
    Vec2 wheelPos;        // 轮子世界位置

    // 计算悬挂力（返回施加在底盘连接点方向的力）
    Vec2 computeForce(const Vec2& chassisWorldPos, const Vec2& wheelWorldPos,
                      float relativeVelocity, float dt) {
        if (state.broken) return {0, 0};

        Vec2 diff = wheelWorldPos - chassisWorldPos;
        float currentLength = diff.length();
        if (currentLength < 1e-6f) return {0, 0};

        Vec2 dir = diff.normalized();

        // 压缩量 = 自然长度 - 当前长度（正值表示压缩）
        state.compression = params.restLength - currentLength;
        state.compression = clamp(state.compression, -params.maxExtension, params.maxCompression);
        state.compressionRatio = (state.compression + params.maxExtension)
                                 / (params.maxCompression + params.maxExtension);
        state.velocity = relativeVelocity;

        // 弹簧力 + 阻尼力
        float springForce = params.stiffness * state.compression;
        float dampingForce = params.damping * relativeVelocity;
        state.force = springForce + dampingForce;

        // 检查是否断裂
        if (std::abs(state.force) > params.breakForce) {
            state.broken = true;
            return {0, 0};
        }

        // 力方向沿悬挂轴（向上为正力推开车轮）
        return dir * (-state.force);
    }
};

} // namespace CrashBox
