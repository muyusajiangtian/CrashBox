#pragma once
// 动态天气系统 - 雨天、结冰、水坑影响车辆物理

#include "MathUtils.h"
#include <vector>
#include <random>
#include <cmath>

namespace CrashBox {

// 天气类型
enum class WeatherType {
    CLEAR,   // 晴天
    RAIN,    // 下雨
    ICE,     // 结冰
    THAW     // 渐融过渡（冰→正常）
};

// 水坑定义
struct WaterPuddle {
    float centerX;       // 水坑中心X
    float width;         // 水坑宽度
    float depth;         // 水坑深度（影响溅水量）
    bool active = true;
};

// 天气系统
class WeatherSystem {
public:
    WeatherType currentWeather = WeatherType::CLEAR;
    float weatherTimer = 0.0f;        // 当前天气持续时间
    float weatherDuration = 30.0f;    // 每种天气持续时间
    float transitionProgress = 0.0f;  // 过渡进度 0~1
    float thawProgress = 0.0f;        // 融冰进度 0~1

    // 摩擦系数修正
    float frictionMultiplier = 1.0f;
    // 侧向抓地力修正
    float lateralGripMultiplier = 1.0f;

    // 水坑
    std::vector<WaterPuddle> puddles;
    // 溅水效果（记录最近被经过的水坑，用于影响后车）
    struct SplashEvent {
        float x;             // 溅水位置
        float timer;         // 剩余持续时间
        int sourceVehicleId; // 产生溅水的车辆ID
    };
    std::vector<SplashEvent> activeSplashes;

    // 雨滴粒子（视觉效果）
    struct RainDrop {
        Vec2 position;
        Vec2 velocity;
        float life;
    };
    std::vector<RainDrop> rainDrops;

    void init(float trackLength, unsigned int seed = 42) {
        // 在赛道上随机放置水坑
        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> xDist(10.0f, trackLength - 10.0f);
        std::uniform_real_distribution<float> wDist(1.5f, 4.0f);
        std::uniform_real_distribution<float> dDist(0.05f, 0.15f);

        int puddleCount = static_cast<int>(trackLength / 25.0f);
        for (int i = 0; i < puddleCount; ++i) {
            WaterPuddle p;
            p.centerX = xDist(rng);
            p.width = wDist(rng);
            p.depth = dDist(rng);
            puddles.push_back(p);
        }
    }

    void update(float dt) {
        weatherTimer += dt;

        // 天气自动轮换
        if (weatherTimer >= weatherDuration) {
            weatherTimer = 0.0f;
            cycleWeather();
        }

        // 计算过渡进度
        transitionProgress = std::min(weatherTimer / 5.0f, 1.0f);

        // 根据天气类型更新摩擦系数
        switch (currentWeather) {
            case WeatherType::CLEAR:
                frictionMultiplier = 1.0f;
                lateralGripMultiplier = 1.0f;
                break;
            case WeatherType::RAIN:
                // 雨天摩擦降低到60%，侧向抓地力降低到50%
                frictionMultiplier = lerp(1.0f, 0.6f, transitionProgress);
                lateralGripMultiplier = lerp(1.0f, 0.5f, transitionProgress);
                break;
            case WeatherType::ICE:
                // 结冰摩擦大幅降低到20%
                frictionMultiplier = lerp(0.6f, 0.2f, transitionProgress);
                lateralGripMultiplier = lerp(0.5f, 0.15f, transitionProgress);
                break;
            case WeatherType::THAW:
                // 渐融过渡：从冰面慢慢恢复
                thawProgress = transitionProgress;
                frictionMultiplier = lerp(0.2f, 0.85f, thawProgress);
                lateralGripMultiplier = lerp(0.15f, 0.8f, thawProgress);
                break;
        }

        // 更新溅水事件
        for (auto it = activeSplashes.begin(); it != activeSplashes.end();) {
            it->timer -= dt;
            if (it->timer <= 0.0f) {
                it = activeSplashes.erase(it);
            } else {
                ++it;
            }
        }

        // 更新雨滴粒子
        if (currentWeather == WeatherType::RAIN) {
            updateRainDrops(dt);
        } else {
            rainDrops.clear();
        }
    }

    // 检测车辆是否经过水坑，产生溅水
    void checkPuddleSplash(float vehicleX, float vehicleSpeed, int vehicleId) {
        if (currentWeather != WeatherType::RAIN && currentWeather != WeatherType::CLEAR) return;

        for (auto& puddle : puddles) {
            if (!puddle.active) continue;
            float halfW = puddle.width / 2.0f;
            if (vehicleX >= puddle.centerX - halfW && vehicleX <= puddle.centerX + halfW) {
                if (std::abs(vehicleSpeed) > 5.0f) {
                    SplashEvent splash;
                    splash.x = puddle.centerX;
                    splash.timer = 2.0f; // 溅水持续2秒
                    splash.sourceVehicleId = vehicleId;
                    activeSplashes.push_back(splash);
                }
            }
        }
    }

    // 检测车辆是否被溅水影响（视线遮挡）
    bool isAffectedBySplash(float vehicleX, int vehicleId) const {
        for (auto& splash : activeSplashes) {
            if (splash.sourceVehicleId == vehicleId) continue;
            // 溅水影响范围：后方5米内
            float dist = vehicleX - splash.x;
            if (dist > -1.0f && dist < 5.0f) {
                return true;
            }
        }
        return false;
    }

    // 获取当前天气名称
    const char* getWeatherName() const {
        switch (currentWeather) {
            case WeatherType::CLEAR: return "晴天";
            case WeatherType::RAIN: return "雨天";
            case WeatherType::ICE: return "结冰";
            case WeatherType::THAW: return "融冰中";
        }
        return "未知";
    }

    // 手动切换天气
    void setWeather(WeatherType type) {
        currentWeather = type;
        weatherTimer = 0.0f;
        transitionProgress = 0.0f;
    }

private:
    void cycleWeather() {
        switch (currentWeather) {
            case WeatherType::CLEAR: currentWeather = WeatherType::RAIN; break;
            case WeatherType::RAIN: currentWeather = WeatherType::ICE; break;
            case WeatherType::ICE: currentWeather = WeatherType::THAW; break;
            case WeatherType::THAW: currentWeather = WeatherType::CLEAR; break;
        }
    }

    void updateRainDrops(float dt) {
        // 生成新雨滴
        int targetCount = 200;
        while ((int)rainDrops.size() < targetCount) {
            RainDrop drop;
            drop.position = {0, 0}; // 渲染时相对相机随机
            drop.velocity = {-1.0f, 8.0f};
            drop.life = 1.0f;
            rainDrops.push_back(drop);
        }
        // 更新现有雨滴
        for (auto it = rainDrops.begin(); it != rainDrops.end();) {
            it->position += it->velocity * dt;
            it->life -= dt;
            if (it->life <= 0.0f) {
                it->life = 1.0f;
                it->position = {0, 0};
            }
            ++it;
        }
    }
};

} // namespace CrashBox
