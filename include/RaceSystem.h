#pragma once
// 赛道与排行榜系统 - 环形赛道、圈数检测、实时排名

#include "MathUtils.h"
#include <vector>
#include <string>
#include <algorithm>

namespace CrashBox {

// 检查点
struct Checkpoint {
    float x;           // X坐标位置
    bool isFinishLine; // 是否为终点线（起点=终点）
};

// 单车赛道数据
struct RaceEntry {
    int vehicleId = -1;
    std::string name;
    int currentLap = 0;          // 当前圈数
    int lastCheckpoint = -1;     // 最后经过的检查点
    float positionX = 0.0f;      // 当前X位置
    float speedKmh = 0.0f;       // 当前速度
    float totalDistance = 0.0f;  // 总行驶距离（用于精确排名）
    float lapTime = 0.0f;        // 当前圈用时
    float bestLapTime = 999.0f;  // 最佳圈时间
    bool finished = false;       // 是否完赛
};

// 排名结果
struct RankEntry {
    int vehicleId;
    int rank;
    int lap;
    float speed;
    std::string name;
    float gap; // 与第一名的距离差
};

class RaceSystem {
public:
    std::vector<Checkpoint> checkpoints;
    std::vector<RaceEntry> entries;
    int totalLaps = 3;             // 总圈数
    float trackLength = 100.0f;    // 赛道长度
    bool raceStarted = false;
    bool raceFinished = false;
    float raceTime = 0.0f;

    // 初始化赛道检查点
    void init(float trackLen, int laps) {
        trackLength = trackLen;
        totalLaps = laps;
        checkpoints.clear();
        entries.clear();
        raceStarted = false;
        raceFinished = false;
        raceTime = 0.0f;

        // 均匀分布检查点
        int numCheckpoints = 5;
        for (int i = 0; i < numCheckpoints; ++i) {
            Checkpoint cp;
            cp.x = (trackLen / numCheckpoints) * i;
            cp.isFinishLine = (i == 0);
            checkpoints.push_back(cp);
        }
    }

    // 注册车辆
    void registerVehicle(int id, const std::string& name) {
        RaceEntry entry;
        entry.vehicleId = id;
        entry.name = name;
        entries.push_back(entry);
    }

    // 开始比赛
    void startRace() {
        raceStarted = true;
        raceTime = 0.0f;
        for (auto& e : entries) {
            e.currentLap = 0;
            e.lastCheckpoint = -1;
            e.totalDistance = 0.0f;
            e.lapTime = 0.0f;
            e.finished = false;
        }
    }

    // 更新（每帧调用）
    void update(float dt) {
        if (!raceStarted || raceFinished) return;
        raceTime += dt;
        for (auto& e : entries) {
            if (!e.finished) {
                e.lapTime += dt;
            }
        }
    }

    // 更新车辆位置
    void updateVehiclePosition(int vehicleId, float posX, float speedKmh) {
        if (!raceStarted) return;

        RaceEntry* entry = findEntry(vehicleId);
        if (!entry || entry->finished) return;

        float prevX = entry->positionX;
        entry->positionX = posX;
        entry->speedKmh = speedKmh;

        // 累计距离（环形赛道：从右到左回到起点，按X增长计）
        float dx = posX - prevX;
        if (dx > 0) {
            entry->totalDistance += dx;
        }

        // 检查点检测（简化：按X位置顺序经过）
        for (int i = 0; i < (int)checkpoints.size(); ++i) {
            if (i == entry->lastCheckpoint) continue;
            int nextCp = (entry->lastCheckpoint + 1) % (int)checkpoints.size();
            if (i != nextCp) continue;

            float cpX = checkpoints[i].x;
            // 经过检查点判定
            if (prevX < cpX && posX >= cpX) {
                entry->lastCheckpoint = i;

                // 经过终点线
                if (checkpoints[i].isFinishLine && entry->lastCheckpoint == 0 &&
                    entry->totalDistance > trackLength * 0.5f) {
                    entry->currentLap++;
                    if (entry->lapTime < entry->bestLapTime) {
                        entry->bestLapTime = entry->lapTime;
                    }
                    entry->lapTime = 0.0f;

                    if (entry->currentLap >= totalLaps) {
                        entry->finished = true;
                    }
                }
                break;
            }
        }

        // 检查比赛是否全部完成
        bool allFinished = true;
        for (auto& e : entries) {
            if (!e.finished) { allFinished = false; break; }
        }
        if (allFinished) raceFinished = true;
    }

    // 获取排名（按总距离排序）
    std::vector<RankEntry> getRankings() const {
        std::vector<RankEntry> rankings;

        struct SortHelper {
            int id;
            int lap;
            float dist;
            float speed;
            std::string name;
        };
        std::vector<SortHelper> helpers;

        for (auto& e : entries) {
            helpers.push_back({e.vehicleId, e.currentLap, e.totalDistance,
                              e.speedKmh, e.name});
        }

        // 排序：圈数优先，同圈按距离
        std::sort(helpers.begin(), helpers.end(), [](const SortHelper& a, const SortHelper& b) {
            if (a.lap != b.lap) return a.lap > b.lap;
            return a.dist > b.dist;
        });

        float leaderDist = helpers.empty() ? 0.0f : helpers[0].dist;
        for (int i = 0; i < (int)helpers.size(); ++i) {
            RankEntry r;
            r.vehicleId = helpers[i].id;
            r.rank = i + 1;
            r.lap = helpers[i].lap;
            r.speed = helpers[i].speed;
            r.name = helpers[i].name;
            r.gap = leaderDist - helpers[i].dist;
            rankings.push_back(r);
        }

        return rankings;
    }

private:
    RaceEntry* findEntry(int vehicleId) {
        for (auto& e : entries) {
            if (e.vehicleId == vehicleId) return &e;
        }
        return nullptr;
    }
};

} // namespace CrashBox
