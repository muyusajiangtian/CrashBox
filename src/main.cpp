// CrashBox - 2D车辆物理模拟器 主程序
// 多车对抗 + 动态天气 + 漂移 + 拖车绳 + 排行榜

#ifdef _WIN32
#include <windows.h>
#endif

#include "../include/MathUtils.h"
#include "../include/Vehicle.h"
#include "../include/Tire.h"
#include "../include/Suspension.h"
#include "../include/Terrain.h"
#include "../include/Collision.h"
#include "../include/Renderer.h"
#include "../include/Config.h"
#include "../include/Logger.h"
#include "../include/Weather.h"
#include "../include/Drift.h"
#include "../include/TowRope.h"
#include "../include/VehicleCollision.h"
#include "../include/AIController.h"
#include "../include/RaceSystem.h"
#include "../include/GameWorld.h"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>

using namespace CrashBox;

int main() {
#ifdef _WIN32
    // 设置控制台输出编码为UTF-8，解决中文乱码
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    // 初始化日志
    Logger::instance().init("crashbox.log");
    LOG_INFO("CrashBox 多车对抗版 启动中...");

    // 加载配置
    Config config;
    if (!config.loadFromFile("config/config.json")) {
        LOG_WARN("使用默认配置");
    }

    // 创建窗口
    int winWidth = config.getInt("window_width", 1280);
    int winHeight = config.getInt("window_height", 720);
    std::string title = config.getString("window_title", "CrashBox - 多车对抗");
    int fpsLimit = config.getInt("fps_limit", 60);

    sf::RenderWindow window(sf::VideoMode({(unsigned)winWidth, (unsigned)winHeight}),
                            sf::String::fromUtf8(title.begin(), title.end()));
    window.setFramerateLimit(fpsLimit);
    LOG_INFO("窗口创建完成: " + std::to_string(winWidth) + "x" + std::to_string(winHeight));

    // 初始化渲染器
    Renderer renderer;
    config.applyToRenderer(renderer.config);
    std::string fontPath = config.getString("font_path", "C:/Windows/Fonts/msyh.ttc");
    if (!renderer.loadFont(fontPath)) {
        LOG_WARN("字体加载失败: " + fontPath + "，尝试备用字体");
        if (!renderer.loadFont("C:/Windows/Fonts/simhei.ttf")) {
            LOG_ERROR("所有字体加载失败，HUD将不显示文字");
        }
    }

    // 初始化游戏世界
    GameWorld world;
    world.init(config);
    LOG_INFO("游戏世界初始化完成：4辆车（1玩家+3AI）");

    // 物理参数
    float physicsDt = config.getFloat("physics_dt", 0.001f);
    int substeps = config.getInt("physics_substeps", 10);

    // 相机初始位置
    auto* player = world.getPlayerVehicle();
    if (player) {
        Vec2 initCom = player->vehicle.chassis.getCenterOfMass();
        renderer.cameraPos = {initCom.x, initCom.y};
    }

    // 主循环
    sf::Clock clock;
    LOG_INFO("进入主循环 - 多车对抗模式");

    while (window.isOpen()) {
        // 事件处理
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (auto* keyEvt = event->getIf<sf::Event::KeyPressed>()) {
                if (keyEvt->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
                // R键重置
                if (keyEvt->code == sf::Keyboard::Key::R) {
                    world.reset(config);
                    LOG_INFO("游戏世界已重置");
                }
                // G键切换天气
                if (keyEvt->code == sf::Keyboard::Key::G) {
                    world.cycleWeather();
                    LOG_INFO(std::string("天气切换为: ") + world.weather.getWeatherName());
                }
                // T键切换地形
                if (keyEvt->code == sf::Keyboard::Key::T) {
                    world.terrain.generateRandom(-5.0f, 150.0f, 0.0f, 3.0f, 0.3f);
                    world.reset(config);
                    LOG_INFO("切换到随机地形并重置");
                }
                // C键切换拖车绳（连接玩家和最近的AI）
                if (keyEvt->code == sf::Keyboard::Key::C) {
                    if (world.vehicles.size() > 1) {
                        world.toggleTowRope(0, 1);
                        if (world.towRope.state.connected) {
                            LOG_INFO("拖车绳已连接");
                        } else {
                            LOG_INFO("拖车绳已断开");
                        }
                    }
                }
            }
        }

        // 玩家键盘输入
        player = world.getPlayerVehicle();
        if (player) {
            player->vehicle.throttleInput = 0.0f;
            player->vehicle.brakeInput = 0.0f;
            player->vehicle.steerInput = 0.0f;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
                player->vehicle.throttleInput = 1.0f;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
                player->vehicle.brakeInput = 1.0f;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
                player->vehicle.steerInput = -1.0f;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
                player->vehicle.steerInput = 1.0f;
            }
        }

        // 物理更新
        float frameTime = clock.restart().asSeconds();
        frameTime = std::min(frameTime, 0.05f);
        world.update(physicsDt, substeps);

        // 更新相机跟随玩家
        if (player) {
            Vec2 vehicleCenter = player->vehicle.chassis.getCenterOfMass();
            renderer.updateCamera(vehicleCenter, frameTime);
        }

        // 渲染
        window.clear(renderer.config.backgroundColor);

        // 天气效果背景层（增强版需要dt和terrain）
        renderer.drawWeatherEffects(window, world.weather, frameTime, world.terrain);

        // 地形
        renderer.drawTerrain(window, world.terrain);

        // 水坑溅水粒子生成（车辆经过水坑时）
        for (auto& vi : world.vehicles) {
            Vec2 pos = vi.vehicle.chassis.getCenterOfMass();
            for (auto& puddle : world.weather.puddles) {
                float halfW = puddle.width / 2.0f;
                if (pos.x >= puddle.centerX - halfW && pos.x <= puddle.centerX + halfW) {
                    if (std::abs(vi.vehicle.speed) > 3.0f) {
                        auto winSize = window.getSize();
                        sf::Vector2f sp = renderer.worldToScreen(
                            {pos.x, world.terrain.getHeightAt(pos.x)}, winSize);
                        renderer.spawnWaterSplash(sp);
                        // 轮印
                        renderer.addTireTrack(sp);
                    }
                }
            }
        }

        // 所有车辆
        for (auto& vi : world.vehicles) {
            renderer.drawVehicleColored(window, vi.vehicle, vi.color, vi.name);
            renderer.drawDriftEffect(window, vi.vehicle, vi.drift.state);
        }

        // 拖车绳
        if (world.towRope.state.connected && world.vehicles.size() > 1) {
            int idA = world.towRope.state.vehicleA;
            int idB = world.towRope.state.vehicleB;
            Vec2 posA = {0, 0}, posB = {0, 0};
            for (auto& v : world.vehicles) {
                if (v.id == idA) posA = v.vehicle.chassis.points[3].position;
                if (v.id == idB) posB = v.vehicle.chassis.points[5].position;
            }
            renderer.drawTowRope(window, posA, posB, world.towRope.state);
        }

        // 溅水遮挡效果
        if (player) {
            bool blinded = world.weather.isAffectedBySplash(
                player->vehicle.chassis.getCenterOfMass().x, player->id);
            renderer.drawSplashBlind(window, blinded);
        }

        // HUD
        if (player) {
            renderer.drawHUD(window, player->vehicle,
                            player->drift.state, world.weather);
        }

        // 排行榜
        auto rankings = world.raceSystem.getRankings();
        renderer.drawLeaderboard(window, rankings, 0);

        window.display();
    }

    LOG_INFO("CrashBox 正常退出");
    return 0;
}
