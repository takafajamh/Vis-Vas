#include "CsvGyroLoader.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace
{
std::runtime_error MalformedRow(const std::filesystem::path& path, std::size_t lineNumber,
                                std::string_view reason)
{
    std::ostringstream message;
    message << "Malformed gyro CSV row at " << path.string() << ':' << lineNumber << ": " << reason;
    return std::runtime_error(message.str());
}

std::vector<std::string_view> SplitRow(std::string_view row)
{
    std::vector<std::string_view> fields;
    std::size_t start = 0;
    while (true)
    {
        const std::size_t separator = row.find(';', start);
        fields.push_back(row.substr(start, separator == std::string_view::npos ? separator : separator - start));
        if (separator == std::string_view::npos)
            break;
        start = separator + 1;
    }
    return fields;
}

std::uint64_t ParseTimestamp(std::string_view value, const std::filesystem::path& path, std::size_t lineNumber)
{
    if (value.empty())
        throw MalformedRow(path, lineNumber, "timestamp is empty");

    std::uint64_t timestamp = 0;
    const char* begin = value.data();
    const char* end = begin + value.size();
    const auto result = std::from_chars(begin, end, timestamp);
    if (result.ec != std::errc{} || result.ptr != end)
        throw MalformedRow(path, lineNumber, "timestamp is not an unsigned integer");
    return timestamp;
}

double ParseGyroValue(std::string_view value, const std::filesystem::path& path, std::size_t lineNumber)
{
    if (value.empty())
        throw MalformedRow(path, lineNumber, "gyro value is empty");

    std::string normalized(value);
    std::replace(normalized.begin(), normalized.end(), ',', '.');

    std::size_t parsedLength = 0;
    double result = 0.0;
    try
    {
        result = std::stod(normalized, &parsedLength);
    }
    catch (const std::invalid_argument&)
    {
        throw MalformedRow(path, lineNumber, "gyro value is not a number");
    }
    catch (const std::out_of_range&)
    {
        throw MalformedRow(path, lineNumber, "gyro value is out of range");
    }

    if (parsedLength != normalized.size() || !std::isfinite(result))
        throw MalformedRow(path, lineNumber, "gyro value is not a finite number");
    return result;
}
} // namespace

GyroRecording LoadGyroCsv(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input.is_open())
    {
        std::ostringstream message;
        message << "Unable to open gyro CSV file: " << path.string();
        throw std::runtime_error(message.str());
    }

    GyroRecording recording;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line))
    {
        ++lineNumber;
        const std::vector<std::string_view> fields = SplitRow(line);
        if (fields.size() != 6)
            throw MalformedRow(path, lineNumber, "expected exactly six semicolon-delimited fields");

        const std::uint64_t timestamp = ParseTimestamp(fields[0], path, lineNumber);
        const std::string sensorId(fields[2]);
        if (sensorId.empty())
            throw MalformedRow(path, lineNumber, "sensor ID is empty");
        if (sensorId == "TEMP")
            continue;

        recording.samples.push_back(
            {sensorId, timestamp, ParseGyroValue(fields[3], path, lineNumber),
             ParseGyroValue(fields[4], path, lineNumber), ParseGyroValue(fields[5], path, lineNumber)});
    }

    if (input.bad())
    {
        std::ostringstream message;
        message << "Unable to read gyro CSV file: " << path.string();
        throw std::runtime_error(message.str());
    }
    if (recording.samples.empty())
        throw std::runtime_error("Gyro CSV contains no motion samples after filtering: " + path.string());

    std::sort(recording.samples.begin(), recording.samples.end(),
              [](const GyroSample& left, const GyroSample& right)
              {
                  if (left.timestampMs != right.timestampMs)
                      return left.timestampMs < right.timestampMs;
                  return left.sensorId < right.sensorId;
              });

    for (const GyroSample& sample : recording.samples)
    {
        if (std::find(recording.sensorIds.begin(), recording.sensorIds.end(), sample.sensorId) ==
            recording.sensorIds.end())
            recording.sensorIds.push_back(sample.sensorId);
    }
    std::sort(recording.sensorIds.begin(), recording.sensorIds.end());

    const std::uint64_t firstTimestamp = recording.samples.front().timestampMs;
    for (GyroSample& sample : recording.samples)
        sample.timestampMs -= firstTimestamp;

    recording.durationSeconds = static_cast<float>(
        static_cast<long double>(recording.samples.back().timestampMs) / 1000.0L);
    return recording;
}
