#pragma once
// 渲染模块 - 使用SFML绘制车辆、地形、HUD

#include "MathUtils.h"
#include "Vehicle.h"
#include "Terrain.h"
#include <SFML/Graphics.hpp>
#include <SFML/System/String.hpp>
#include <string>
#include <sstream>
#include <iomanip>

namespace CrashBox {

// 渲染配置
struct RenderConfig {
    float pixelsPerMeter = 40.0f;    // 缩放比例
    float cameraSmooth = 5.0f;       // 相机平滑跟随速度
    sf::Color backgroundColor = sf::Color(30, 30, 40);
    sf::Color terrainColor = sf::Color(80, 160, 80);
    sf::Color chassisColor = sf::Color(200, 60, 60);
    sf::Color springColor = sf::Color(255, 200, 100);
    sf::Color brokenSpringColor = sf::Color(100, 100, 100, 128);
    sf::Color wheelColor = sf::Color(50, 50, 50);
    sf::Color hudColor = sf::Color(220, 220, 220);
    sf::Color warningColor = sf::Color(255, 80, 80);
};

class Renderer {
public:
    RenderConfig config;
    Vec2 cameraPos = {0, 0};
    sf::Font font;
    bool fontLoaded = false;

    bool loadFont(const std::string& path) {
        auto result = font.openFromFile(path);
        fontLoaded = result;
        return result;
    }

    // 世界坐标到屏幕坐标
    sf::Vector2f worldToScreen(Vec2 world, sf::Vector2u windowSize) const {
        float sx = (world.x - cameraPos.x) * config.pixelsPerMeter + windowSize.x / 2.0f;
        float sy = (world.y - cameraPos.y) * config.pixelsPerMeter + windowSize.y / 2.0f;
        return {sx, sy};
    }

    // 更新相机跟随
    void updateCamera(Vec2 target, float dt) {
        float t = 1.0f - std::exp(-config.cameraSmooth * dt);
        cameraPos.x = lerp(cameraPos.x, target.x, t);
        cameraPos.y = lerp(cameraPos.y, target.y - 2.0f, t); // 稍微偏上
    }

    // 绘制地形
    void drawTerrain(sf::RenderWindow& window, const Terrain& terrain) {
        auto winSize = window.getSize();
        for (auto& seg : terrain.segments) {
            sf::Vector2f p1 = worldToScreen(seg.start, winSize);
            sf::Vector2f p2 = worldToScreen(seg.end, winSize);

            // 地形线
            std::array<sf::Vertex, 2> line = {
                sf::Vertex{p1, config.terrainColor},
                sf::Vertex{p2, config.terrainColor}
            };
            window.draw(line.data(), line.size(), sf::PrimitiveType::Lines);

            // 地形填充（向下填充）
            float bottomY = static_cast<float>(winSize.y) + 50.0f;
            sf::ConvexShape fill;
            fill.setPointCount(4);
            fill.setPoint(0, p1);
            fill.setPoint(1, p2);
            fill.setPoint(2, {p2.x, bottomY});
            fill.setPoint(3, {p1.x, bottomY});
            fill.setFillColor(sf::Color(40, 100, 40, 180));
            window.draw(fill);
        }
    }

    // 绘制车辆底盘（弹簧网络）
    void drawVehicle(sf::RenderWindow& window, const Vehicle& vehicle) {
        auto winSize = window.getSize();

        // 绘制弹簧
        for (auto& spring : vehicle.chassis.springs) {
            Vec2 pa = vehicle.chassis.points[spring.pointA].position;
            Vec2 pb = vehicle.chassis.points[spring.pointB].position;
            sf::Vector2f sa = worldToScreen(pa, winSize);
            sf::Vector2f sb = worldToScreen(pb, winSize);

            sf::Color color = spring.broken ? config.brokenSpringColor : config.springColor;
            // 根据形变量改变颜色
            if (!spring.broken) {
                float dist = (pa - pb).length();
                float strain = std::abs(dist - spring.restLength) / spring.restLength;
                if (strain > 0.3f) {
                    color = sf::Color(255, 100, 50);
                } else if (strain > 0.1f) {
                    color = sf::Color(255, 180, 50);
                }
            }

            std::array<sf::Vertex, 2> line = {
                sf::Vertex{sa, color},
                sf::Vertex{sb, color}
            };
            window.draw(line.data(), line.size(), sf::PrimitiveType::Lines);
        }

        // 绘制质点
        for (auto& point : vehicle.chassis.points) {
            sf::Vector2f sp = worldToScreen(point.position, winSize);
            sf::CircleShape circle(4.0f);
            circle.setFillColor(config.chassisColor);
            circle.setPosition(sp - sf::Vector2f{4.0f, 4.0f});
            window.draw(circle);
        }

        // 绘制轮子
        for (int i = 0; i < 2; ++i) {
            if (!vehicle.wheelAttached[i]) continue;

            sf::Vector2f wp = worldToScreen(vehicle.wheelPositions[i], winSize);
            float r = vehicle.config.tireRadius * config.pixelsPerMeter;
            sf::CircleShape wheel(r);
            wheel.setFillColor(config.wheelColor);
            wheel.setOutlineColor(sf::Color(150, 150, 150));
            wheel.setOutlineThickness(2.0f);
            wheel.setPosition(wp - sf::Vector2f{r, r});
            window.draw(wheel);

            // 轮子旋转指示线
            float angle = vehicle.tires[i].state.angularVelocity * 0.1f;
            sf::Vector2f lineEnd = wp + sf::Vector2f{
                std::cos(angle) * r * 0.8f,
                std::sin(angle) * r * 0.8f
            };
            std::array<sf::Vertex, 2> spoke = {
                sf::Vertex{wp, sf::Color(200, 200, 200)},
                sf::Vertex{lineEnd, sf::Color(200, 200, 200)}
            };
            window.draw(spoke.data(), spoke.size(), sf::PrimitiveType::Lines);

            // 悬挂连接线
            int anchor = vehicle.wheelAnchorIndices[i];
            sf::Vector2f ap = worldToScreen(vehicle.chassis.points[anchor].position, winSize);
            sf::Color suspColor = vehicle.suspBroken[i] ?
                sf::Color(255, 0, 0) : sf::Color(100, 200, 255);
            std::array<sf::Vertex, 2> suspLine = {
                sf::Vertex{ap, suspColor},
                sf::Vertex{wp, suspColor}
            };
            window.draw(suspLine.data(), suspLine.size(), sf::PrimitiveType::Lines);
        }
    }

    // 绘制HUD信息
    void drawHUD(sf::RenderWindow& window, const Vehicle& vehicle) {
        if (!fontLoaded) return;

        auto winSize = window.getSize();
        float x = 10.0f, y = 10.0f;
        float lineHeight = 22.0f;
        unsigned int fontSize = 16;

        // UTF-8字符串转sf::String
        auto toSfStr = [](const std::string& s) -> sf::String {
            return sf::String::fromUtf8(s.begin(), s.end());
        };

        auto drawText = [&](const std::string& str, sf::Color color = sf::Color(220, 220, 220)) {
            sf::Text text(font, toSfStr(str), fontSize);
            text.setFillColor(color);
            text.setPosition({x, y});
            window.draw(text);
            y += lineHeight;
        };

        // 速度
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);
        oss << "速度: " << vehicle.speedKmh << " km/h";
        drawText(oss.str());

        // G力
        oss.str(""); oss.clear();
        oss << "G力: X=" << vehicle.gForce.x << " Y=" << vehicle.gForce.y;
        sf::Color gColor = (std::abs(vehicle.gForce.x) > 2.0f || std::abs(vehicle.gForce.y) > 2.0f)
            ? config.warningColor : config.hudColor;
        drawText(oss.str(), gColor);

        // 悬挂压缩
        oss.str(""); oss.clear();
        oss << "悬挂: 前=";
        if (vehicle.suspBroken[0]) {
            oss << "[断裂]";
        } else {
            oss << std::setprecision(0) << (vehicle.suspCompression[0] * 100) << "%";
        }
        oss << " 后=";
        if (vehicle.suspBroken[1]) {
            oss << "[断裂]";
        } else {
            oss << std::setprecision(0) << (vehicle.suspCompression[1] * 100) << "%";
        }
        drawText(oss.str());

        // 损坏程度
        oss.str(""); oss.clear();
        float damage = vehicle.getDamageLevel();
        oss << "损坏: " << std::setprecision(0) << (damage * 100) << "%";
        sf::Color dmgColor = damage > 0.5f ? config.warningColor :
                            (damage > 0.2f ? sf::Color(255, 200, 50) : config.hudColor);
        drawText(oss.str(), dmgColor);

        // 轮胎滑移
        oss.str(""); oss.clear();
        oss << std::setprecision(2);
        oss << "滑移: " << vehicle.tires[0].state.slipRatio;
        drawText(oss.str());

        // 操作提示（右下角）
        float rx = static_cast<float>(winSize.x) - 250.0f;
        float ry = static_cast<float>(winSize.y) - 130.0f;
        x = rx; y = ry;
        drawText("--- 操作说明 ---", sf::Color(180, 180, 200));
        drawText("W/↑: 加速", sf::Color(150, 150, 170));
        drawText("S/↓: 刹车", sf::Color(150, 150, 170));
        drawText("A/←: 左移  D/→: 右移", sf::Color(150, 150, 170));
        drawText("R: 重置  T: 切换地形", sf::Color(150, 150, 170));
        drawText("ESC: 退出", sf::Color(150, 150, 170));
    }
};

} // namespace CrashBox
