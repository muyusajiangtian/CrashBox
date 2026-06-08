#pragma once
// 轮胎模块 - 简化Pacejka模型实现纵向/侧向抓地力

#include "MathUtils.h"

namespace CrashBox {

// Pacejka魔术公式参数
struct PacejkaParams {
    float B = 10.0f;   // 刚度因子
    float C = 1.9f;    // 形状因子
    float D = 1.0f;    // 峰值因子（归一化摩擦系数）
    float E = -0.5f;   // 曲率因子
};

// 轮胎状态
struct TireState {
    float slipRatio = 0.0f;      // 纵向滑移率
    float slipAngle = 0.0f;      // 侧偏角(弧度)
    float longitudinalForce = 0.0f; // 纵向力
    float lateralForce = 0.0f;   // 侧向力
    float normalLoad = 0.0f;     // 法向载荷
    float angularVelocity = 0.0f; // 角速度
};

class Tire {
public:
    PacejkaParams longitudinalParams; // 纵向参数
    PacejkaParams lateralParams;      // 侧向参数
    TireState state;
    float radius = 0.3f;              // 轮胎半径(米)
    float inertia = 1.0f;             // 转动惯量
    bool attached = true;             // 是否还连接在车上

    Tire() {
        lateralParams.B = 12.0f;
        lateralParams.C = 1.3f;
        lateralParams.D = 1.0f;
        lateralParams.E = -0.6f;
    }

    // Pacejka魔术公式: F = D * sin(C * atan(B*x - E*(B*x - atan(B*x))))
    static float pacejka(float slip, const PacejkaParams& p) {
        float Bx = p.B * slip;
        return p.D * std::sin(p.C * std::atan(Bx - p.E * (Bx - std::atan(Bx))));
    }

    // 计算轮胎力
    void computeForces(float vLongitudinal, float vLateral, float normalLoad_,
                       float brakeTorque, float driveTorque, float dt) {
        state.normalLoad = normalLoad_;
        if (normalLoad_ < 1.0f || !attached) {
            state.longitudinalForce = 0;
            state.lateralForce = 0;
            return;
        }

        // 计算纵向滑移率
        float wheelSpeed = state.angularVelocity * radius;
        float absVx = std::abs(vLongitudinal);
        float denom = std::max(absVx, std::abs(wheelSpeed));
        if (denom < 0.5f) denom = 0.5f;
        state.slipRatio = (wheelSpeed - vLongitudinal) / denom;
        state.slipRatio = clamp(state.slipRatio, -1.0f, 1.0f);

        // 计算侧偏角
        if (absVx > 0.5f) {
            state.slipAngle = std::atan2(vLateral, absVx);
        } else {
            state.slipAngle = 0.0f;
        }

        // Pacejka计算归一化力
        float muLong = pacejka(state.slipRatio, longitudinalParams);
        float muLat = pacejka(state.slipAngle, lateralParams);

        // 联合滑移（简化椭圆模型）
        float totalMu = std::sqrt(muLong * muLong + muLat * muLat);
        if (totalMu > 1.0f) {
            muLong /= totalMu;
            muLat /= totalMu;
        }

        state.longitudinalForce = muLong * normalLoad_;
        state.lateralForce = -muLat * normalLoad_;

        // 更新角速度（驱动力矩 - 制动力矩 - 滚动阻力）
        float netTorque = driveTorque - brakeTorque * (state.angularVelocity > 0 ? 1.0f : -1.0f)
                          - state.longitudinalForce * radius;
        state.angularVelocity += (netTorque / inertia) * dt;

        // 防止刹车时反转
        if (brakeTorque > 0 && std::abs(state.angularVelocity) < 0.1f) {
            state.angularVelocity = 0;
        }
    }
};

} // namespace CrashBox
