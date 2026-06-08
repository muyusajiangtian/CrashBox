#pragma once
// 渲染模块 - 使用SFML绘制多车、地形、天气效果、排行榜HUD

#include "MathUtils.h"
#include "Vehicle.h"
#include "Terrain.h"
#include "Weather.h"
#include "Drift.h"
#include "TowRope.h"
#include "RaceSystem.h"
#include <SFML/Graphics.hpp>
#include <SFML/System/String.hpp>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <random>

namespace CrashBox {

// 前向声明
struct VehicleInstance;

// 渲染配置
struct RenderConfig {
    float pixelsPerMeter = 40.0f;
    float cameraSmooth = 5.0f;
    sf::Color backgroundColor = sf::Color(30, 30, 40);
    sf::Color terrainColor = sf::Color(80, 160, 80);
    sf::Color chassisColor = sf::Color(200, 60, 60);
    sf::Color springColor = sf::Color(255, 200, 100);
    sf::Color brokenSpringColor = sf::Color(100, 100, 100, 128);
    sf::Color wheelColor = sf::Color(50, 50, 50);
    sf::Color hudColor = sf::Color(220, 220, 220);
    sf::Color warningColor = sf::Color(255, 80, 80);
};

// 持久化雨滴粒子
struct RainParticle {
    float x, y;        // 屏幕坐标
    float speed;       // 下落速度
    float length;      // 雨滴长度
    float windOffset;  // 风偏移
};

// 地面溅射粒子
struct GroundSplash {
    float x, y;
    float timer;
    float size;
};

// 水花粒子（车辆经过水坑时）
struct WaterSplashParticle {
    float x, y;
    float vx, vy;
    float life;
    float maxLife;
};

// 轮印记录
struct TireTrack {
    float x, y;
    float alpha;  // 渐隐
};

class Renderer {
public:
    RenderConfig config;
    Vec2 cameraPos = {0, 0};
    sf::Font font;
    bool fontLoaded = false;

    // 雨滴粒子系统
    std::mt19937 rainRng{42};
    std::vector<RainParticle> rainParticles;
    std::vector<GroundSplash> groundSplashes;
    bool rainInitialized = false;

    // 水花粒子
    std::vector<WaterSplashParticle> waterSplashParticles;

    // 轮印
    std::vector<TireTrack> tireTracks;

    // 冰面闪烁计时
    float iceShimmerTimer = 0.0f;

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
        cameraPos.y = lerp(cameraPos.y, target.y - 2.0f, t);
    }

    // 绘制地形
    void drawTerrain(sf::RenderWindow& window, const Terrain& terrain) {
        auto winSize = window.getSize();
        for (auto& seg : terrain.segments) {
            sf::Vector2f p1 = worldToScreen(seg.start, winSize);
            sf::Vector2f p2 = worldToScreen(seg.end, winSize);

            std::array<sf::Vertex, 2> line = {
                sf::Vertex{p1, config.terrainColor},
                sf::Vertex{p2, config.terrainColor}
            };
            window.draw(line.data(), line.size(), sf::PrimitiveType::Lines);

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

    // 绘制带颜色的车辆
    void drawVehicleColored(sf::RenderWindow& window, const Vehicle& vehicle,
                            sf::Color bodyColor, const std::string& label) {
        auto winSize = window.getSize();

        // 绘制弹簧
        for (auto& spring : vehicle.chassis.springs) {
            Vec2 pa = vehicle.chassis.points[spring.pointA].position;
            Vec2 pb = vehicle.chassis.points[spring.pointB].position;
            sf::Vector2f sa = worldToScreen(pa, winSize);
            sf::Vector2f sb = worldToScreen(pb, winSize);

            sf::Color color = spring.broken ? config.brokenSpringColor : config.springColor;
            if (!spring.broken) {
                float dist = (pa - pb).length();
                float strain = std::abs(dist - spring.restLength) / spring.restLength;
                if (strain > 0.3f) color = sf::Color(255, 100, 50);
                else if (strain > 0.1f) color = sf::Color(255, 180, 50);
            }

            std::array<sf::Vertex, 2> line = {
                sf::Vertex{sa, color},
                sf::Vertex{sb, color}
            };
            window.draw(line.data(), line.size(), sf::PrimitiveType::Lines);
        }

        // 绘制质点（用车辆专属颜色）
        for (auto& point : vehicle.chassis.points) {
            sf::Vector2f sp = worldToScreen(point.position, winSize);
            sf::CircleShape circle(4.0f);
            circle.setFillColor(bodyColor);
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

        // 车辆名称标签
        if (fontLoaded && !label.empty()) {
            Vec2 center = vehicle.chassis.getCenterOfMass();
            sf::Vector2f labelPos = worldToScreen({center.x, center.y - 1.5f}, winSize);
            auto toSfStr = [](const std::string& s) -> sf::String {
                return sf::String::fromUtf8(s.begin(), s.end());
            };
            sf::Text text(font, toSfStr(label), 12);
            text.setFillColor(bodyColor);
            text.setPosition(labelPos);
            window.draw(text);
        }
    }

    // 绘制漂移痕迹（视觉效果）
    void drawDriftEffect(sf::RenderWindow& window, const Vehicle& vehicle,
                         const DriftState& driftState) {
        if (!driftState.isDrifting) return;
        auto winSize = window.getSize();

        // 后轮位置画烟雾
        Vec2 rearWheel = vehicle.wheelPositions[1];
        sf::Vector2f sp = worldToScreen(rearWheel, winSize);

        float alpha = driftState.driftIntensity * 150.0f;
        sf::CircleShape smoke(8.0f + driftState.driftIntensity * 5.0f);
        smoke.setFillColor(sf::Color(180, 180, 180, (uint8_t)alpha));
        smoke.setPosition(sp - sf::Vector2f{8.0f, 8.0f});
        window.draw(smoke);
    }

    // 绘制拖车绳
    void drawTowRope(sf::RenderWindow& window, Vec2 posA, Vec2 posB,
                     const TowRopeState& state) {
        if (!state.connected) return;
        auto winSize = window.getSize();

        sf::Vector2f sa = worldToScreen(posA, winSize);
        sf::Vector2f sb = worldToScreen(posB, winSize);

        // 绳索颜色随张力变化
        sf::Color ropeColor = sf::Color(200, 150, 50);
        if (state.tension > 50000.0f) ropeColor = sf::Color(255, 100, 50);
        else if (state.tension > 20000.0f) ropeColor = sf::Color(255, 200, 50);

        std::array<sf::Vertex, 2> line = {
            sf::Vertex{sa, ropeColor},
            sf::Vertex{sb, ropeColor}
        };
        window.draw(line.data(), line.size(), sf::PrimitiveType::Lines);
    }

    // 绘制天气效果（增强版 - 明显可见的雨/冰/水坑）
    void drawWeatherEffects(sf::RenderWindow& window, const WeatherSystem& weather,
                            float dt, const Terrain& terrain) {
        auto winSize = window.getSize();
        float winW = (float)winSize.x;
        float winH = (float)winSize.y;

        // ====== 雨天：大量可见雨滴从上方落下 + 地面溅射 ======
        if (weather.currentWeather == WeatherType::RAIN) {
            // 初始化雨滴
            if (!rainInitialized || rainParticles.size() < 400) {
                initRainParticles(winW, winH);
                rainInitialized = true;
            }

            // 雨天背景变暗
            sf::RectangleShape darkOverlay(sf::Vector2f(winW, winH));
            darkOverlay.setFillColor(sf::Color(0, 0, 30, 60));
            darkOverlay.setPosition({0, 0});
            window.draw(darkOverlay);

            // 更新和绘制雨滴
            std::uniform_real_distribution<float> xReset(0.0f, winW);
            std::uniform_real_distribution<float> splashChance(0.0f, 1.0f);

            for (auto& drop : rainParticles) {
                // 更新位置
                drop.y += drop.speed * dt * 60.0f;
                drop.x += drop.windOffset * dt * 60.0f;

                // 超出屏幕底部：重置到顶部，并可能产生地面溅射
                if (drop.y > winH) {
                    if (splashChance(rainRng) < 0.3f) {
                        GroundSplash gs;
                        gs.x = drop.x;
                        gs.y = winH * 0.75f + splashChance(rainRng) * winH * 0.2f;
                        gs.timer = 0.3f;
                        gs.size = 2.0f + splashChance(rainRng) * 3.0f;
                        groundSplashes.push_back(gs);
                    }
                    drop.y = -drop.length;
                    drop.x = xReset(rainRng);
                }

                // 绘制雨滴线条（较长、明显）
                float alpha = 180.0f;
                sf::Vector2f start(drop.x, drop.y);
                sf::Vector2f end(drop.x + drop.windOffset * 0.5f, drop.y + drop.length);
                std::array<sf::Vertex, 2> line = {
                    sf::Vertex{start, sf::Color(160, 200, 255, (uint8_t)alpha)},
                    sf::Vertex{end, sf::Color(120, 160, 220, (uint8_t)(alpha * 0.4f))}
                };
                window.draw(line.data(), line.size(), sf::PrimitiveType::Lines);
            }

            // 绘制地面溅射
            for (auto it = groundSplashes.begin(); it != groundSplashes.end();) {
                it->timer -= dt;
                if (it->timer <= 0) {
                    it = groundSplashes.erase(it);
                    continue;
                }
                float progress = 1.0f - it->timer / 0.3f;
                float r = it->size * (1.0f + progress * 2.0f);
                uint8_t a = (uint8_t)(180 * (1.0f - progress));

                // 溅射环
                sf::CircleShape ring(r);
                ring.setFillColor(sf::Color::Transparent);
                ring.setOutlineColor(sf::Color(180, 210, 255, a));
                ring.setOutlineThickness(1.5f);
                ring.setPosition({it->x - r, it->y - r});
                window.draw(ring);

                // 小水滴向上弹
                for (int d = 0; d < 3; ++d) {
                    float dx = (d - 1) * r * 0.6f;
                    float dy = -progress * 8.0f * (1.0f + d * 0.3f);
                    sf::CircleShape droplet(1.5f);
                    droplet.setFillColor(sf::Color(180, 220, 255, a));
                    droplet.setPosition({it->x + dx, it->y + dy});
                    window.draw(droplet);
                }
                ++it;
            }
        } else {
            rainInitialized = false;
            rainParticles.clear();
            groundSplashes.clear();
        }

        // ====== 冰面：明显的泛白+反光条纹+冰晶闪烁 ======
        if (weather.currentWeather == WeatherType::ICE ||
            weather.currentWeather == WeatherType::THAW) {

            float intensity = (weather.currentWeather == WeatherType::ICE) ?
                              weather.transitionProgress :
                              (1.0f - weather.thawProgress);

            iceShimmerTimer += dt;

            // 整体泛白叠加（明显的白色/浅蓝，不是之前的弱蓝色）
            uint8_t whiteAlpha = (uint8_t)(100.0f * intensity);
            sf::RectangleShape whiteOverlay(sf::Vector2f(winW, winH));
            whiteOverlay.setFillColor(sf::Color(200, 220, 255, whiteAlpha));
            whiteOverlay.setPosition({0, 0});
            window.draw(whiteOverlay);

            // 冰面反光条纹：沿地形线绘制高亮白色条纹
            for (auto& seg : terrain.segments) {
                sf::Vector2f p1 = worldToScreen(seg.start, winSize);
                sf::Vector2f p2 = worldToScreen(seg.end, winSize);

                // 反光白线（地形表面的冰层）
                uint8_t iceA = (uint8_t)(200.0f * intensity);
                std::array<sf::Vertex, 2> iceLine = {
                    sf::Vertex{p1 + sf::Vector2f{0, -2.0f}, sf::Color(220, 240, 255, iceA)},
                    sf::Vertex{p2 + sf::Vector2f{0, -2.0f}, sf::Color(220, 240, 255, iceA)}
                };
                window.draw(iceLine.data(), iceLine.size(), sf::PrimitiveType::Lines);

                // 更宽的半透明冰面带
                sf::ConvexShape iceStrip;
                iceStrip.setPointCount(4);
                iceStrip.setPoint(0, p1 + sf::Vector2f{0, -4.0f});
                iceStrip.setPoint(1, p2 + sf::Vector2f{0, -4.0f});
                iceStrip.setPoint(2, p2 + sf::Vector2f{0, 3.0f});
                iceStrip.setPoint(3, p1 + sf::Vector2f{0, 3.0f});
                iceStrip.setFillColor(sf::Color(200, 230, 255, (uint8_t)(80.0f * intensity)));
                window.draw(iceStrip);
            }

            // 冰晶闪烁点（模拟冰面反光闪烁）
            std::uniform_real_distribution<float> shimmerX(0.0f, winW);
            std::uniform_real_distribution<float> shimmerY(winH * 0.4f, winH * 0.85f);
            std::uniform_real_distribution<float> shimmerSize(1.0f, 4.0f);
            int shimmerCount = (int)(30 * intensity);
            float shimmerPhase = iceShimmerTimer * 3.0f;
            for (int i = 0; i < shimmerCount; ++i) {
                float sx = shimmerX(rainRng);
                float sy = shimmerY(rainRng);
                float ss = shimmerSize(rainRng);
                // 用sin制造闪烁
                float flicker = 0.5f + 0.5f * std::sin(shimmerPhase + i * 1.7f);
                uint8_t sa = (uint8_t)(200.0f * intensity * flicker);
                sf::CircleShape sparkle(ss);
                sparkle.setFillColor(sf::Color(255, 255, 255, sa));
                sparkle.setPosition({sx, sy});
                window.draw(sparkle);
            }
        }

        // ====== 水坑：明显的水面反光和深色区域 ======
        for (auto& puddle : weather.puddles) {
            Vec2 left = {puddle.centerX - puddle.width / 2.0f, 0.0f};
            Vec2 right = {puddle.centerX + puddle.width / 2.0f, 0.0f};
            sf::Vector2f sl = worldToScreen(left, winSize);
            sf::Vector2f sr = worldToScreen(right, winSize);
            float pw = sr.x - sl.x;
            if (pw < 2.0f) continue;

            float puddleY = sl.y;

            // 水坑暗色底层
            sf::RectangleShape puddleBase(sf::Vector2f(pw, 8.0f));
            puddleBase.setFillColor(sf::Color(20, 50, 120, 160));
            puddleBase.setPosition({sl.x, puddleY - 4.0f});
            window.draw(puddleBase);

            // 水面反光高亮
            sf::RectangleShape highlight(sf::Vector2f(pw * 0.6f, 2.0f));
            highlight.setFillColor(sf::Color(180, 220, 255, 140));
            highlight.setPosition({sl.x + pw * 0.2f, puddleY - 3.0f});
            window.draw(highlight);

            // 水坑边缘渐变
            sf::RectangleShape edgeL(sf::Vector2f(pw * 0.15f, 6.0f));
            edgeL.setFillColor(sf::Color(40, 80, 150, 100));
            edgeL.setPosition({sl.x, puddleY - 3.0f});
            window.draw(edgeL);
            sf::RectangleShape edgeR(sf::Vector2f(pw * 0.15f, 6.0f));
            edgeR.setFillColor(sf::Color(40, 80, 150, 100));
            edgeR.setPosition({sr.x - pw * 0.15f, puddleY - 3.0f});
            window.draw(edgeR);

            // 雨天时水坑有涟漪
            if (weather.currentWeather == WeatherType::RAIN) {
                std::uniform_real_distribution<float> rx(sl.x, sr.x);
                for (int r = 0; r < 3; ++r) {
                    float rippleX = rx(rainRng);
                    float rippleR = 3.0f + std::sin(iceShimmerTimer * 5.0f + r * 2.0f) * 2.0f;
                    sf::CircleShape ripple(rippleR);
                    ripple.setFillColor(sf::Color::Transparent);
                    ripple.setOutlineColor(sf::Color(180, 210, 255, 80));
                    ripple.setOutlineThickness(1.0f);
                    ripple.setPosition({rippleX - rippleR, puddleY - rippleR - 2.0f});
                    window.draw(ripple);
                }
            }
        }

        // ====== 水花溅起粒子（车辆经过水坑产生） ======
        for (auto it = waterSplashParticles.begin(); it != waterSplashParticles.end();) {
            it->life -= dt;
            if (it->life <= 0) {
                it = waterSplashParticles.erase(it);
                continue;
            }
            // 物理更新
            it->x += it->vx * dt * 60.0f;
            it->y += it->vy * dt * 60.0f;
            it->vy += 0.3f * dt * 60.0f; // 重力

            float lifeRatio = it->life / it->maxLife;
            uint8_t a = (uint8_t)(220.0f * lifeRatio);
            float size = 2.0f + (1.0f - lifeRatio) * 3.0f;

            sf::CircleShape particle(size);
            particle.setFillColor(sf::Color(120, 180, 255, a));
            particle.setPosition({it->x - size, it->y - size});
            window.draw(particle);
            ++it;
        }

        // ====== 活跃溅水事件（大水花效果） ======
        for (auto& splash : weather.activeSplashes) {
            Vec2 splashWorld = {splash.x, 0.0f};
            sf::Vector2f sp = worldToScreen(splashWorld, winSize);
            float t = splash.timer; // 2.0 -> 0.0

            // 大水花弧形
            for (int i = 0; i < 8; ++i) {
                float angle = -PI * 0.1f - (PI * 0.8f) * (i / 7.0f);
                float dist = 15.0f + t * 10.0f;
                float px = sp.x + std::cos(angle) * dist;
                float py = sp.y + std::sin(angle) * dist * 0.6f;
                uint8_t a = (uint8_t)(200.0f * (t / 2.0f));
                float r = 3.0f * (t / 2.0f);
                sf::CircleShape drop(r);
                drop.setFillColor(sf::Color(100, 170, 255, a));
                drop.setPosition({px - r, py - r});
                window.draw(drop);
            }

            // 中心水花柱
            float colH = 25.0f * (t / 2.0f);
            sf::RectangleShape column(sf::Vector2f(4.0f, colH));
            column.setFillColor(sf::Color(130, 190, 255, (uint8_t)(150.0f * t / 2.0f)));
            column.setPosition({sp.x - 2.0f, sp.y - colH});
            window.draw(column);
        }

        // ====== 轮印拖痕渐隐 ======
        for (auto it = tireTracks.begin(); it != tireTracks.end();) {
            it->alpha -= dt * 30.0f;
            if (it->alpha <= 0) {
                it = tireTracks.erase(it);
                continue;
            }
            sf::RectangleShape mark(sf::Vector2f(3.0f, 2.0f));
            mark.setFillColor(sf::Color(30, 60, 100, (uint8_t)it->alpha));
            mark.setPosition({it->x, it->y});
            window.draw(mark);
            ++it;
        }
    }

    // 生成水花粒子（被GameWorld调用，当车辆经过水坑时）
    void spawnWaterSplash(sf::Vector2f screenPos) {
        std::uniform_real_distribution<float> vxDist(-3.0f, 3.0f);
        std::uniform_real_distribution<float> vyDist(-5.0f, -1.5f);
        std::uniform_real_distribution<float> lifeDist(0.4f, 0.9f);
        for (int i = 0; i < 12; ++i) {
            WaterSplashParticle p;
            p.x = screenPos.x + vxDist(rainRng) * 2.0f;
            p.y = screenPos.y;
            p.vx = vxDist(rainRng);
            p.vy = vyDist(rainRng);
            p.maxLife = lifeDist(rainRng);
            p.life = p.maxLife;
            waterSplashParticles.push_back(p);
        }
    }

    // 添加轮印
    void addTireTrack(sf::Vector2f screenPos) {
        TireTrack t;
        t.x = screenPos.x;
        t.y = screenPos.y;
        t.alpha = 150.0f;
        tireTracks.push_back(t);
        // 限制轮印数量
        if (tireTracks.size() > 200) {
            tireTracks.erase(tireTracks.begin());
        }
    }

    // 绘制溅水遮挡效果（影响视线）
    void drawSplashBlind(sf::RenderWindow& window, bool isBlinded) {
        if (!isBlinded) return;
        auto winSize = window.getSize();
        float winW = (float)winSize.x;
        float winH = (float)winSize.y;

        // 大块水渍遮挡
        std::uniform_real_distribution<float> xDist(winW * 0.1f, winW * 0.9f);
        std::uniform_real_distribution<float> yDist(winH * 0.1f, winH * 0.5f);
        std::uniform_real_distribution<float> sDist(15.0f, 50.0f);
        for (int i = 0; i < 8; ++i) {
            float rx = xDist(rainRng);
            float ry = yDist(rainRng);
            float rs = sDist(rainRng);
            sf::CircleShape blob(rs);
            blob.setFillColor(sf::Color(80, 140, 220, 110));
            blob.setPosition({rx - rs, ry - rs});
            window.draw(blob);
            // 内部高光
            sf::CircleShape highlight(rs * 0.4f);
            highlight.setFillColor(sf::Color(150, 200, 255, 70));
            highlight.setPosition({rx - rs * 0.4f, ry - rs * 0.5f});
            window.draw(highlight);
        }

        // 水流拖痕
        for (int i = 0; i < 5; ++i) {
            float sx = xDist(rainRng);
            float sy = yDist(rainRng) * 0.5f;
            sf::RectangleShape streak(sf::Vector2f(3.0f, 40.0f + sDist(rainRng)));
            streak.setFillColor(sf::Color(100, 160, 230, 60));
            streak.setPosition({sx, sy});
            window.draw(streak);
        }
    }

    // 绘制排行榜HUD
    void drawLeaderboard(sf::RenderWindow& window, const std::vector<RankEntry>& rankings,
                         int playerVehicleId) {
        if (!fontLoaded) return;
        auto winSize = window.getSize();

        auto toSfStr = [](const std::string& s) -> sf::String {
            return sf::String::fromUtf8(s.begin(), s.end());
        };

        float x = static_cast<float>(winSize.x) - 280.0f;
        float y = 10.0f;
        float lineHeight = 20.0f;

        // 标题
        sf::Text title(font, toSfStr("--- 排行榜 ---"), 14);
        title.setFillColor(sf::Color(255, 220, 100));
        title.setPosition({x, y});
        window.draw(title);
        y += lineHeight + 5.0f;

        for (auto& r : rankings) {
            std::ostringstream oss;
            oss << "P" << r.rank << " " << r.name
                << " L" << r.lap
                << " " << std::fixed << std::setprecision(0) << r.speed << "km/h";

            if (r.gap > 0.01f) {
                oss << " +" << std::setprecision(1) << r.gap << "m";
            }

            sf::Color color = (r.vehicleId == playerVehicleId) ?
                sf::Color(255, 255, 100) : sf::Color(200, 200, 200);

            sf::Text entry(font, toSfStr(oss.str()), 13);
            entry.setFillColor(color);
            entry.setPosition({x, y});
            window.draw(entry);
            y += lineHeight;
        }
    }

    // 绘制主HUD（玩家车辆信息）
    void drawHUD(sf::RenderWindow& window, const Vehicle& vehicle,
                 const DriftState& driftState, const WeatherSystem& weather) {
        if (!fontLoaded) return;

        float x = 10.0f, y = 10.0f;
        float lineHeight = 22.0f;
        unsigned int fontSize = 16;

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

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);

        // 速度
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
        if (vehicle.suspBroken[0]) oss << "[断裂]";
        else oss << std::setprecision(0) << (vehicle.suspCompression[0] * 100) << "%";
        oss << " 后=";
        if (vehicle.suspBroken[1]) oss << "[断裂]";
        else oss << std::setprecision(0) << (vehicle.suspCompression[1] * 100) << "%";
        drawText(oss.str());

        // 损坏
        oss.str(""); oss.clear();
        float damage = vehicle.getDamageLevel();
        oss << "损坏: " << std::setprecision(0) << (damage * 100) << "%";
        sf::Color dmgColor = damage > 0.5f ? config.warningColor :
                            (damage > 0.2f ? sf::Color(255, 200, 50) : config.hudColor);
        drawText(oss.str(), dmgColor);

        // 漂移状态
        if (driftState.isDrifting) {
            oss.str(""); oss.clear();
            oss << "漂移! 强度: " << std::setprecision(0)
                << (driftState.driftIntensity * 100) << "%";
            drawText(oss.str(), sf::Color(255, 150, 50));
        }

        // 天气
        oss.str(""); oss.clear();
        oss << "天气: " << weather.getWeatherName()
            << " 抓地: " << std::setprecision(0)
            << (weather.frictionMultiplier * 100) << "%";
        sf::Color weatherColor = weather.frictionMultiplier < 0.5f ?
            config.warningColor : config.hudColor;
        drawText(oss.str(), weatherColor);

        // 操作提示（右下角）
        auto winSize = window.getSize();
        float rx = static_cast<float>(winSize.x) - 280.0f;
        float ry = static_cast<float>(winSize.y) - 160.0f;
        x = rx; y = ry;
        drawText("--- 操作说明 ---", sf::Color(180, 180, 200));
        drawText("WASD/方向键: 驾驶", sf::Color(150, 150, 170));
        drawText("R: 重置  G: 切换天气", sf::Color(150, 150, 170));
        drawText("T: 切换地形  C: 拖车绳", sf::Color(150, 150, 170));
        drawText("1-3: 撞击对应AI", sf::Color(150, 150, 170));
        drawText("ESC: 退出", sf::Color(150, 150, 170));
    }

private:
    void initRainParticles(float winW, float winH) {
        rainParticles.clear();
        std::uniform_real_distribution<float> xDist(0.0f, winW);
        std::uniform_real_distribution<float> yDist(-winH, winH);
        std::uniform_real_distribution<float> speedDist(6.0f, 14.0f);
        std::uniform_real_distribution<float> lenDist(12.0f, 30.0f);
        std::uniform_real_distribution<float> windDist(-1.5f, -0.5f);
        for (int i = 0; i < 400; ++i) {
            RainParticle p;
            p.x = xDist(rainRng);
            p.y = yDist(rainRng);
            p.speed = speedDist(rainRng);
            p.length = lenDist(rainRng);
            p.windOffset = windDist(rainRng);
            rainParticles.push_back(p);
        }
    }
};

} // namespace CrashBox
