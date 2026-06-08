#pragma once
// 碰撞与损坏模块 - 质点弹簧软体变形系统（含数值稳定性保护）

#include "MathUtils.h"
#include <vector>
#include <cmath>

namespace CrashBox {

struct MassPoint {
    Vec2 position;
    Vec2 velocity;
    Vec2 force;
    float mass = 100.0f;
    bool fixed = false;
};

struct Spring {
    int pointA = 0;
    int pointB = 0;
    float restLength = 1.0f;
    float stiffness = 100000.0f;
    float damping = 1000.0f;
    float breakThreshold = 2.5f;
    bool broken = false;
};

class SoftBody {
public:
    std::vector<MassPoint> points;
    std::vector<Spring> springs;

    static constexpr float MAX_VELOCITY = 50.0f;   // 最大速度限制(m/s)
    static constexpr float MAX_FORCE = 500000.0f;   // 最大力限制(N)

    int addPoint(Vec2 pos, float mass_) {
        MassPoint p;
        p.position = pos;
        p.mass = mass_;
        points.push_back(p);
        return static_cast<int>(points.size() - 1);
    }

    void addSpring(int a, int b, float stiffness_, float damping_, float breakThreshold_) {
        Spring s;
        s.pointA = a;
        s.pointB = b;
        s.restLength = (points[a].position - points[b].position).length();
        s.stiffness = stiffness_;
        s.damping = damping_;
        s.breakThreshold = breakThreshold_;
        springs.push_back(s);
    }

    void clearForces() {
        for (auto& p : points) p.force = {0, 0};
    }

    void applyForce(int index, Vec2 force) {
        if (index >= 0 && index < (int)points.size()) {
            points[index].force += force;
        }
    }

    void applyGravity(float gravity) {
        for (auto& p : points) {
            if (!p.fixed) {
                p.force.y += p.mass * gravity;
            }
        }
    }

    void computeSpringForces() {
        for (auto& s : springs) {
            if (s.broken) continue;

            Vec2 diff = points[s.pointB].position - points[s.pointA].position;
            float dist = diff.length();
            if (dist < 1e-5f) continue;

            Vec2 dir = diff / dist;
            float stretch = dist - s.restLength;

            // 弹簧力（限幅防爆）
            float springF = s.stiffness * stretch;
            springF = clamp(springF, -MAX_FORCE, MAX_FORCE);

            // 阻尼力
            Vec2 relVel = points[s.pointB].velocity - points[s.pointA].velocity;
            float dampF = s.damping * relVel.dot(dir);
            dampF = clamp(dampF, -MAX_FORCE * 0.5f, MAX_FORCE * 0.5f);

            float totalForce = springF + dampF;
            Vec2 forceVec = dir * totalForce;

            points[s.pointA].force += forceVec;
            points[s.pointB].force -= forceVec;

            // 断裂检测
            if (dist > s.restLength * s.breakThreshold) {
                s.broken = true;
            }
        }
    }

    // 半隐式欧拉积分（含安全限幅）
    void integrate(float dt) {
        for (auto& p : points) {
            if (p.fixed) continue;

            // 限制力
            float fLen = p.force.length();
            if (fLen > MAX_FORCE) {
                p.force = p.force * (MAX_FORCE / fLen);
            }

            // NaN检测
            if (std::isnan(p.force.x) || std::isnan(p.force.y)) {
                p.force = {0, 0};
            }

            Vec2 acc = p.force / p.mass;
            p.velocity += acc * dt;

            // 限制速度
            float vLen = p.velocity.length();
            if (vLen > MAX_VELOCITY) {
                p.velocity = p.velocity * (MAX_VELOCITY / vLen);
            }

            if (std::isnan(p.velocity.x) || std::isnan(p.velocity.y)) {
                p.velocity = {0, 0};
            }

            p.position += p.velocity * dt;
        }
    }

    Vec2 getCenterOfMass() const {
        Vec2 com = {0, 0};
        float totalMass = 0;
        for (auto& p : points) {
            com += p.position * p.mass;
            totalMass += p.mass;
        }
        if (totalMass > 0) com = com / totalMass;
        return com;
    }

    Vec2 getAverageVelocity() const {
        Vec2 vel = {0, 0};
        float totalMass = 0;
        for (auto& p : points) {
            vel += p.velocity * p.mass;
            totalMass += p.mass;
        }
        if (totalMass > 0) vel = vel / totalMass;
        return vel;
    }

    float getTotalDeformation() const {
        float total = 0;
        for (auto& s : springs) {
            if (s.broken) { total += s.restLength; continue; }
            float dist = (points[s.pointB].position - points[s.pointA].position).length();
            total += std::abs(dist - s.restLength);
        }
        return total;
    }

    int brokenSpringCount() const {
        int count = 0;
        for (auto& s : springs) { if (s.broken) ++count; }
        return count;
    }
};

} // namespace CrashBox
