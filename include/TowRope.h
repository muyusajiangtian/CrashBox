#pragma once
// 拖车绳系统 - 车辆之间弹性绳约束

#include "MathUtils.h"
#include <cmath>

namespace CrashBox {

// 拖车绳参数
struct TowRopeParams {
    float maxLength = 5.0f;          // 绳索最大长度(米)
    float stiffness = 20000.0f;      // 绳索刚度(N/m)
    float damping = 2000.0f;         // 绳索阻尼
    float breakForce = 100000.0f;    // 断裂力(N)
};

// 拖车绳状态
struct TowRopeState {
    bool connected = false;     // 是否连接
    bool broken = false;        // 是否断裂
    int vehicleA = -1;          // 前车（拖车）ID
    int vehicleB = -1;          // 后车（被拖车）ID
    float currentLength = 0.0f; // 当前绳长
    float tension = 0.0f;       // 当前张力
};

class TowRope {
public:
    TowRopeParams params;
    TowRopeState state;

    // 连接两辆车
    void connect(int idA, int idB) {
        state.connected = true;
        state.broken = false;
        state.vehicleA = idA;
        state.vehicleB = idB;
        state.tension = 0.0f;
    }

    // 断开连接
    void disconnect() {
        state.connected = false;
        state.vehicleA = -1;
        state.vehicleB = -1;
        state.tension = 0.0f;
    }

    // 计算绳索力（返回施加给后车的力方向和大小）
    // posA: 前车后端连接点位置
    // posB: 后车前端连接点位置
    // velA: 前车速度
    // velB: 后车速度
    Vec2 computeForce(Vec2 posA, Vec2 posB, Vec2 velA, Vec2 velB) {
        if (!state.connected || state.broken) return {0, 0};

        Vec2 diff = posA - posB;
        float dist = diff.length();
        state.currentLength = dist;

        // 绳索只在拉伸时产生力（不能推）
        if (dist <= params.maxLength) {
            state.tension = 0.0f;
            return {0, 0};
        }

        Vec2 dir = diff.normalized();
        float stretch = dist - params.maxLength;

        // 弹性力
        float springForce = params.stiffness * stretch;

        // 阻尼力（沿绳索方向的相对速度）
        Vec2 relVel = velA - velB;
        float dampForce = params.damping * relVel.dot(dir);

        float totalForce = springForce + dampForce;
        totalForce = std::max(0.0f, totalForce); // 绳索只拉不推

        state.tension = totalForce;

        // 检查断裂
        if (totalForce > params.breakForce) {
            state.broken = true;
            state.connected = false;
            return {0, 0};
        }

        // 力方向：拉后车向前车方向
        return dir * totalForce;
    }
};

} // namespace CrashBox
