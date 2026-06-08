// CrashBox - 2D车辆物理模拟器 主程序
// 整合所有模块，实现游戏主循环

#include "../include/MathUtils.h"
#include "../include/Vehicle.h"
#include "../include/Tire.h"
#include "../include/Suspension.h"
#include "../include/Terrain.h"
#include "../include/Collision.h"
#include "../include/Renderer.h"
#include "../include/Config.h"
#include "../include/Logger.h"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>

using namespace CrashBox;

int main() {
    // 初始化日志
    Logger::instance().init("crashbox.log");
    LOG_INFO("CrashBox 启动中...");

    // 加载配置
    Config config;
    if (!config.loadFromFile("config/config.json")) {
        LOG_WARN("使用默认配置");
    }

    // 创建窗口
    int winWidth = config.getInt("window_width", 1280);
    int winHeight = config.getInt("window_height", 720);
    std::string title = config.getString("window_title", "CrashBox");
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

    // 初始化地形
    Terrain terrain;
    std::string terrainType = config.getString("terrain_type", "preset");
    bool usePreset = (terrainType == "preset");
    if (usePreset) {
        terrain.generatePresetTrack();
        LOG_INFO("已加载预设赛道");
    } else {
        terrain.generateRandom(-5.0f,
            config.getFloat("terrain_length", 200.0f),
            0.0f,
            config.getFloat("terrain_segment_width", 3.0f),
            config.getFloat("terrain_max_slope", 0.3f));
        LOG_INFO("已生成随机地形");
    }

    // 初始化车辆
    Vehicle vehicle;
    config.applyToVehicle(vehicle.config);
    Vec2 startPos = {5.0f, 0.0f};
    vehicle.init(startPos, terrain);
    LOG_INFO("车辆初始化完成");

    // 物理参数
    float physicsDt = config.getFloat("physics_dt", 0.001f);
    int substeps = config.getInt("physics_substeps", 10);

    // 相机初始位置
    Vec2 initCom = vehicle.chassis.getCenterOfMass();
    renderer.cameraPos = {initCom.x, initCom.y};

    // 主循环
    sf::Clock clock;
    LOG_INFO("进入主循环");

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
                // R键重置车辆
                if (keyEvt->code == sf::Keyboard::Key::R) {
                    startPos = {5.0f, 0.0f};
                    vehicle = Vehicle();
                    config.applyToVehicle(vehicle.config);
                    vehicle.init(startPos, terrain);
                    LOG_INFO("车辆已重置");
                }
                // T键切换地形
                if (keyEvt->code == sf::Keyboard::Key::T) {
                    usePreset = !usePreset;
                    if (usePreset) {
                        terrain.generatePresetTrack();
                        LOG_INFO("切换到预设赛道");
                    } else {
                        terrain.generateRandom(-5.0f, 200.0f, 0.0f, 3.0f, 0.3f);
                        LOG_INFO("切换到随机地形");
                    }
                    // 重置车辆到新地形
                    startPos = {5.0f, 0.0f};
                    vehicle = Vehicle();
                    config.applyToVehicle(vehicle.config);
                    vehicle.init(startPos, terrain);
                }
            }
        }

        // 键盘输入
        vehicle.throttleInput = 0.0f;
        vehicle.brakeInput = 0.0f;
        vehicle.steerInput = 0.0f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
            vehicle.throttleInput = 1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
            vehicle.brakeInput = 1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
            vehicle.steerInput = -1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
            vehicle.steerInput = 1.0f;
        }

        // 物理更新（固定时间步）
        float frameTime = clock.restart().asSeconds();
        frameTime = std::min(frameTime, 0.05f); // 防止大跳跃

        for (int i = 0; i < substeps; ++i) {
            vehicle.update(physicsDt, terrain);
        }

        // 更新相机
        Vec2 vehicleCenter = vehicle.chassis.getCenterOfMass();
        renderer.updateCamera(vehicleCenter, frameTime);

        // 渲染
        window.clear(renderer.config.backgroundColor);
        renderer.drawTerrain(window, terrain);
        renderer.drawVehicle(window, vehicle);
        renderer.drawHUD(window, vehicle);
        window.display();
    }

    LOG_INFO("CrashBox 正常退出");
    return 0;
}
