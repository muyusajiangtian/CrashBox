#pragma once
// 车辆间碰撞系统 - AABB检测 + 动量传递 + 碰撞变形

#include "MathUtils.h"
#include "Collision.h"
#include <vector>
#include <cmath>

namespace CrashBox {

// 车辆AABB包围盒
struct VehicleAABB {
    Vec2 min;
    Vec2 max;
};

// 碰撞信息
struct VehicleCollisionInfo {
    bool collided = false;
    int vehicleA = -1;
    int vehicleB = -1;
    Vec2 contactPoint = {0, 0};
    Vec2 contactNormal = {0, 0};  // 从A指向B
    float penetration = 0.0f;
    float impulse = 0.0f;
};

class VehicleCollision {
public:
    float restitution = 0.2f;          // 碰撞恢复系数（低一些避免弹飞）
    float deformationFactor = 0.5f;    // 碰撞变形系数
    float separationForce = 50000.0f;  // 穿透分离力
    float maxImpulse = 15000.0f;       // 最大碰撞冲量限制
    float maxPostCollisionSpeed = 15.0f; // 碰撞后最大速度(m/s)

    // 计算车辆AABB
    static VehicleAABB computeAABB(const SoftBody& chassis) {
        VehicleAABB aabb;
        if (chassis.points.empty()) return aabb;

        aabb.min = chassis.points[0].position;
        aabb.max = chassis.points[0].position;

        for (auto& p : chassis.points) {
            aabb.min.x = std::min(aabb.min.x, p.position.x);
            aabb.min.y = std::min(aabb.min.y, p.position.y);
            aabb.max.x = std::max(aabb.max.x, p.position.x);
            aabb.max.y = std::max(aabb.max.y, p.position.y);
        }
        // 扩展一点余量
        aabb.min.x -= 0.1f; aabb.min.y -= 0.1f;
        aabb.max.x += 0.1f; aabb.max.y += 0.1f;
        return aabb;
    }

    // AABB重叠检测
    static bool aabbOverlap(const VehicleAABB& a, const VehicleAABB& b) {
        return !(a.max.x < b.min.x || b.max.x < a.min.x ||
                 a.max.y < b.min.y || b.max.y < a.min.y);
    }

    // 计算两车碰撞并施加力
    VehicleCollisionInfo resolveCollision(SoftBody& bodyA, SoftBody& bodyB,
                                          float massA, float massB) {
        VehicleCollisionInfo info;

        VehicleAABB aabbA = computeAABB(bodyA);
        VehicleAABB aabbB = computeAABB(bodyB);

        if (!aabbOverlap(aabbA, aabbB)) return info;

        // 质量比（用于分配分离量和冲量）
        float totalMass = massA + massB;
        float ratioA = massB / totalMass;
        float ratioB = massA / totalMass;

        // 遍历所有质点对，处理所有重叠的质点（不只是最近的一对）
        float collisionThreshold = 0.5f;
        bool anyCollision = false;
        Vec2 avgContactPoint = {0, 0};
        Vec2 avgNormal = {0, 0};
        int contactCount = 0;

        for (int i = 0; i < (int)bodyA.points.size(); ++i) {
            for (int j = 0; j < (int)bodyB.points.size(); ++j) {
                Vec2 diff = bodyB.points[j].position - bodyA.points[i].position;
                float dist = diff.length();

                if (dist >= collisionThreshold || dist < 1e-4f) continue;

                anyCollision = true;
                float penetration = collisionThreshold - dist;
                Vec2 normal = diff.normalized();
                if (normal.lengthSq() < 0.01f) normal = {1, 0};

                // 立即按质量比分离这对质点到不重叠位置
                float sepTotal = penetration + 0.02f; // 额外2cm安全间距
                bodyA.points[i].position -= normal * (sepTotal * ratioA);
                bodyB.points[j].position += normal * (sepTotal * ratioB);

                // 施加分离速度（防止下一帧又嵌入）
                float relApproach = (bodyA.points[i].velocity - bodyB.points[j].velocity).dot(normal);
                if (relApproach > 0) {
                    // 正在接近，施加分离速度
                    float sepVel = relApproach * (1.0f + restitution) * 0.5f;
                    sepVel = std::min(sepVel, maxPostCollisionSpeed * 0.5f);
                    bodyA.points[i].velocity -= normal * (sepVel * ratioA);
                    bodyB.points[j].velocity += normal * (sepVel * ratioB);
                }

                avgContactPoint += (bodyA.points[i].position + bodyB.points[j].position) * 0.5f;
                avgNormal += normal;
                contactCount++;
            }
        }

        if (!anyCollision) return info;

        info.collided = true;
        avgContactPoint = avgContactPoint / (float)contactCount;
        avgNormal = avgNormal.normalized();
        if (avgNormal.lengthSq() < 0.01f) avgNormal = {1, 0};
        info.contactPoint = avgContactPoint;
        info.contactNormal = avgNormal;
        info.penetration = collisionThreshold;

        // 整车冲量（基于质心速度，处理整体动量传递）
        Vec2 velA = bodyA.getAverageVelocity();
        Vec2 velB = bodyB.getAverageVelocity();
        Vec2 relVel = velA - velB;
        float relVelNormal = relVel.dot(avgNormal);

        if (relVelNormal > 0.1f) {
            float invMassA = 1.0f / massA;
            float invMassB = 1.0f / massB;
            float j = -(1.0f + restitution) * relVelNormal / (invMassA + invMassB);
            j = clamp(j, -maxImpulse, maxImpulse);
            info.impulse = j;

            // 施加整车冲量
            Vec2 impulseVec = avgNormal * j;
            for (auto& p : bodyA.points) {
                p.velocity -= impulseVec * (ratioA / (float)bodyA.points.size() / p.mass * (totalMass / (float)bodyA.points.size()));
            }
            for (auto& p : bodyB.points) {
                p.velocity += impulseVec * (ratioB / (float)bodyB.points.size() / p.mass * (totalMass / (float)bodyB.points.size()));
            }

            // 碰撞后速度限制
            clampBodySpeed(bodyA, maxPostCollisionSpeed);
            clampBodySpeed(bodyB, maxPostCollisionSpeed);

            // 碰撞变形
            if (std::abs(j) > 5000.0f) {
                // 找最近碰撞点用于变形
                int closestA = 0, closestB = 0;
                float minD = 999.0f;
                for (int i = 0; i < (int)bodyA.points.size(); ++i) {
                    float d = (bodyA.points[i].position - avgContactPoint).lengthSq();
                    if (d < minD) { minD = d; closestA = i; }
                }
                minD = 999.0f;
                for (int j = 0; j < (int)bodyB.points.size(); ++j) {
                    float d = (bodyB.points[j].position - avgContactPoint).lengthSq();
                    if (d < minD) { minD = d; closestB = j; }
                }
                applyDeformation(bodyA, closestA, std::abs(j) * deformationFactor);
                applyDeformation(bodyB, closestB, std::abs(j) * deformationFactor);
            }
        }

        return info;
    }

private:
    // 限制整体质点速度，防止碰撞后弹飞
    void clampBodySpeed(SoftBody& body, float maxSpeed) {
        for (auto& p : body.points) {
            float vLen = p.velocity.length();
            if (vLen > maxSpeed) {
                p.velocity = p.velocity * (maxSpeed / vLen);
            }
        }
    }

    // 将冲量分配到附近质点
    void applyImpulseToNearbyPoints(SoftBody& body, Vec2 contactPos,
                                     Vec2 impulse, float totalMass) {
        float radius = 1.0f;
        float totalWeight = 0.0f;
        std::vector<float> weights(body.points.size(), 0.0f);

        for (int i = 0; i < (int)body.points.size(); ++i) {
            float dist = (body.points[i].position - contactPos).length();
            if (dist < radius) {
                weights[i] = 1.0f - dist / radius;
                totalWeight += weights[i];
            }
        }

        if (totalWeight < 0.01f) return;

        for (int i = 0; i < (int)body.points.size(); ++i) {
            if (weights[i] > 0.0f) {
                float ratio = weights[i] / totalWeight;
                body.points[i].velocity += impulse * ratio / body.points[i].mass;
            }
        }
    }

    // 碰撞变形：缩短碰撞点附近的弹簧自然长度
    void applyDeformation(SoftBody& body, int pointIdx, float force) {
        float deformAmount = force / 500000.0f; // 归一化
        deformAmount = std::min(deformAmount, 0.3f);

        for (auto& s : body.springs) {
            if (s.broken) continue;
            if (s.pointA == pointIdx || s.pointB == pointIdx) {
                s.restLength *= (1.0f - deformAmount * 0.1f);
                s.restLength = std::max(s.restLength, 0.1f);
            }
        }
    }
};

} // namespace CrashBox
