#pragma once
// 配置模块 - 从JSON文件加载配置参数

#include "Vehicle.h"
#include "Renderer.h"
#include "Terrain.h"
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <map>

namespace CrashBox {

// 简易JSON解析（避免外部依赖）
class Config {
public:
    std::map<std::string, float> floatValues;
    std::map<std::string, std::string> stringValues;
    std::map<std::string, int> intValues;

    bool loadFromFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "[配置] 无法打开配置文件: " << path << std::endl;
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        parseSimpleJson(content);
        std::cout << "[配置] 已加载配置文件: " << path << std::endl;
        return true;
    }

    float getFloat(const std::string& key, float defaultVal) const {
        auto it = floatValues.find(key);
        if (it != floatValues.end()) return it->second;
        return defaultVal;
    }

    int getInt(const std::string& key, int defaultVal) const {
        auto it = intValues.find(key);
        if (it != intValues.end()) return it->second;
        return defaultVal;
    }

    std::string getString(const std::string& key, const std::string& defaultVal) const {
        auto it = stringValues.find(key);
        if (it != stringValues.end()) return it->second;
        return defaultVal;
    }

    // 将配置应用到车辆参数
    void applyToVehicle(VehicleConfig& vc) const {
        vc.chassisWidth = getFloat("chassis_width", vc.chassisWidth);
        vc.chassisHeight = getFloat("chassis_height", vc.chassisHeight);
        vc.chassisMass = getFloat("chassis_mass", vc.chassisMass);
        vc.wheelbase = getFloat("wheelbase", vc.wheelbase);
        vc.engineForce = getFloat("engine_force", vc.engineForce);
        vc.brakeForce = getFloat("brake_force", vc.brakeForce);
        vc.maxSteerAngle = getFloat("max_steer_angle", vc.maxSteerAngle);
        vc.suspStiffnessFront = getFloat("suspension_stiffness_front", vc.suspStiffnessFront);
        vc.suspStiffnessRear = getFloat("suspension_stiffness_rear", vc.suspStiffnessRear);
        vc.suspDamping = getFloat("suspension_damping", vc.suspDamping);
        vc.suspBreakForce = getFloat("suspension_break_force", vc.suspBreakForce);
        vc.suspRestLength = getFloat("suspension_rest_length", vc.suspRestLength);
        vc.springStiffness = getFloat("spring_stiffness", vc.springStiffness);
        vc.springDamping = getFloat("spring_damping", vc.springDamping);
        vc.springBreakThreshold = getFloat("spring_break_threshold", vc.springBreakThreshold);
        vc.tireRadius = getFloat("tire_radius", vc.tireRadius);
        vc.cgHeight = getFloat("cg_height", vc.cgHeight);
        vc.rideHeight = getFloat("ride_height", vc.rideHeight);
    }

    // 将配置应用到渲染参数
    void applyToRenderer(RenderConfig& rc) const {
        rc.pixelsPerMeter = getFloat("pixels_per_meter", rc.pixelsPerMeter);
        rc.cameraSmooth = getFloat("camera_smooth", rc.cameraSmooth);
    }

private:
    void parseSimpleJson(const std::string& content) {
        // 简易解析: 逐行寻找 "key": value 模式
        std::istringstream stream(content);
        std::string line;
        while (std::getline(stream, line)) {
            // 去掉空白和逗号
            size_t colonPos = line.find(':');
            if (colonPos == std::string::npos) continue;

            std::string key = extractQuotedString(line.substr(0, colonPos));
            if (key.empty()) continue;

            std::string valueStr = line.substr(colonPos + 1);
            // 去掉逗号和空白
            while (!valueStr.empty() && (valueStr.back() == ',' || valueStr.back() == ' '
                   || valueStr.back() == '\r' || valueStr.back() == '\n'))
                valueStr.pop_back();
            while (!valueStr.empty() && valueStr.front() == ' ')
                valueStr.erase(valueStr.begin());

            // 判断值类型
            if (valueStr.front() == '"') {
                stringValues[key] = extractQuotedString(valueStr);
            } else {
                try {
                    if (valueStr.find('.') != std::string::npos) {
                        floatValues[key] = std::stof(valueStr);
                    } else {
                        intValues[key] = std::stoi(valueStr);
                        floatValues[key] = static_cast<float>(intValues[key]);
                    }
                } catch (...) {
                    // 忽略解析错误
                }
            }
        }
    }

    static std::string extractQuotedString(const std::string& s) {
        size_t first = s.find('"');
        if (first == std::string::npos) return "";
        size_t second = s.find('"', first + 1);
        if (second == std::string::npos) return "";
        return s.substr(first + 1, second - first - 1);
    }
};

} // namespace CrashBox
