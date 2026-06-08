#pragma once
// 地形模块 - 折线段地形，支持随机生成和预设赛道

#include "MathUtils.h"
#include <vector>
#include <random>
#include <string>

namespace CrashBox {

// 地形线段
struct TerrainSegment {
    Vec2 start;
    Vec2 end;

    Vec2 normal() const {
        Vec2 dir = (end - start).normalized();
        return dir.perpCCW(); // 法线朝上
    }

    float slope() const {
        Vec2 d = end - start;
        if (std::abs(d.x) < 1e-6f) return 0;
        return d.y / d.x;
    }

    float slopeAngle() const {
        Vec2 d = end - start;
        return std::atan2(d.y, d.x);
    }
};

class Terrain {
public:
    std::vector<Vec2> points;       // 地形顶点序列
    std::vector<TerrainSegment> segments; // 线段缓存

    void buildSegments() {
        segments.clear();
        for (size_t i = 0; i + 1 < points.size(); ++i) {
            segments.push_back({points[i], points[i + 1]});
        }
    }

    // 生成随机地形
    void generateRandom(float startX, float endX, float baseY, float segmentWidth,
                        float maxSlope, unsigned int seed = 0) {
        points.clear();
        std::mt19937 rng(seed == 0 ? std::random_device{}() : seed);
        std::uniform_real_distribution<float> slopeDist(-maxSlope, maxSlope);

        float x = startX;
        float y = baseY;
        points.push_back({x, y});

        while (x < endX) {
            float dx = segmentWidth;
            float dy = slopeDist(rng) * dx;
            x += dx;
            y += dy;
            // 限制高度范围
            y = clamp(y, baseY - 5.0f, baseY + 5.0f);
            points.push_back({x, y});
        }
        buildSegments();
    }

    // 预设赛道：平坦带坡
    void generatePresetTrack() {
        points.clear();
        // 起始平坦段
        points.push_back({-5.0f, 0.0f});
        points.push_back({10.0f, 0.0f});
        // 缓上坡
        points.push_back({20.0f, -2.0f});
        // 山顶平台
        points.push_back({30.0f, -2.0f});
        // 下坡
        points.push_back({40.0f, 0.5f});
        // 凹坑
        points.push_back({45.0f, 1.5f});
        points.push_back({50.0f, 0.5f});
        // 平坦
        points.push_back({60.0f, 0.0f});
        // 陡上坡
        points.push_back({68.0f, -3.0f});
        // 平台
        points.push_back({75.0f, -3.0f});
        // 下坡到终点
        points.push_back({85.0f, 0.0f});
        points.push_back({100.0f, 0.0f});
        points.push_back({120.0f, 0.0f});
        buildSegments();
    }

    // 获取地形在x坐标处的高度（线性插值）
    float getHeightAt(float x) const {
        if (points.empty()) return 0;
        if (x <= points.front().x) return points.front().y;
        if (x >= points.back().x) return points.back().y;

        for (size_t i = 0; i + 1 < points.size(); ++i) {
            if (x >= points[i].x && x <= points[i + 1].x) {
                float t = (x - points[i].x) / (points[i + 1].x - points[i].x);
                return lerp(points[i].y, points[i + 1].y, t);
            }
        }
        return points.back().y;
    }

    // 获取地形在x坐标处的法线
    Vec2 getNormalAt(float x) const {
        for (size_t i = 0; i < segments.size(); ++i) {
            if (x >= segments[i].start.x && x <= segments[i].end.x) {
                return segments[i].normal();
            }
        }
        return {0, -1}; // 默认朝上
    }

    // 获取地形在x坐标处的坡度角
    float getSlopeAngleAt(float x) const {
        for (size_t i = 0; i < segments.size(); ++i) {
            if (x >= segments[i].start.x && x <= segments[i].end.x) {
                return segments[i].slopeAngle();
            }
        }
        return 0;
    }

    // 碰撞检测：点到地形的穿透深度（正值表示穿透）
    struct ContactInfo {
        bool contact = false;
        float penetration = 0.0f;
        Vec2 normal = {0, -1};
        Vec2 contactPoint = {0, 0};
    };

    ContactInfo checkContact(const Vec2& point) const {
        ContactInfo info;
        float terrainY = getHeightAt(point.x);
        // 注意：Y轴向下为正（屏幕坐标），地形Y越大越低
        if (point.y > terrainY) {
            info.contact = true;
            info.penetration = point.y - terrainY;
            info.normal = getNormalAt(point.x);
            info.contactPoint = {point.x, terrainY};
        }
        return info;
    }
};

} // namespace CrashBox
