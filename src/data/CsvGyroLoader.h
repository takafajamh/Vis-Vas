#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct GyroSample
{
    std::string sensorId;
    std::uint64_t timestampMs = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct GyroRecording
{
    std::vector<GyroSample> samples;
    std::vector<std::string> sensorIds;
    float durationSeconds = 0.0f;
};

GyroRecording LoadGyroCsv(const std::filesystem::path& path);
