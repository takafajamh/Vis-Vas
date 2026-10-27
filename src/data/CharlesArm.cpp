#include "CharlesArm.h"

#include "DataPipeline.h"
#include "ui/Scene.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
struct SensorBinding
{
    const char* id;
    CharlesArm::Joint joint;
};

constexpr SensorBinding SensorMap[] = {
    {"0D", CharlesArm::LeftHand},     {"1D", CharlesArm::RightHand},
    {"2D", CharlesArm::LeftWrist},    {"3D", CharlesArm::RightWrist},
    {"4D", CharlesArm::LeftElbow},    {"5D", CharlesArm::RightElbow},
    {"6D", CharlesArm::LeftShoulder}, {"7D", CharlesArm::RightShoulder},
};

constexpr double GyroScale = 262.0 * 1.5;
}

CharlesArm::CharlesArm(const GyroRecording& recording)
{
    PreCompute(recording);
}

void CharlesArm::SetTime(float time)
{
    m_current = m_track.Sample(time);
}

void CharlesArm::Render()
{
    if (m_current.joints.size() < JointCount)
        return;

    DrawArm(LeftShoulder, LeftElbow, LeftWrist, LeftHand);
    DrawArm(RightShoulder, RightElbow, RightWrist, RightHand);
    Scene::DrawBone(m_current.joints[LeftShoulder], m_current.joints[RightShoulder]);
}

float CharlesArm::GetDuration() const
{
    return m_track.GetDuration();
}

const char* CharlesArm::JointName(int joint)
{
    static constexpr const char* names[JointCount] = {
        "Left hand", "Right hand", "Left wrist", "Right wrist",
        "Left elbow", "Right elbow", "Left shoulder", "Right shoulder"
    };
    return names[joint];
}

Color CharlesArm::JointColor(int joint)
{
    static constexpr Color colors[JointCount] = {
        BLUE, BLUE, GREEN, GREEN, RED, RED, ORANGE, ORANGE
    };
    return colors[joint];
}

void CharlesArm::PreCompute(const GyroRecording& recording)
{
    DataPipeline<GyroSample> pipeline(recording.samples);
    pipeline.Apply();
    const std::vector<GyroSample>& samples = pipeline.GetData();

    std::array<std::vector<const GyroSample*>, JointCount> samplesByJoint;
    for (const GyroSample& sample : samples)
    {
        for (const SensorBinding& binding : SensorMap)
        {
            if (sample.sensorId == binding.id)
            {
                samplesByJoint[binding.joint].push_back(&sample);
                break;
            }
        }
    }

    for (const SensorBinding& binding : SensorMap)
    {
        if (samplesByJoint[binding.joint].empty())
            throw std::runtime_error(std::string("CSV recording is missing required sensor ") + binding.id);
    }

    std::array<std::vector<Angles>, JointCount> anglesByJoint;
    for (int joint = 0; joint < JointCount; ++joint)
    {
        const auto& jointSamples = samplesByJoint[joint];
        auto& jointAngles = anglesByJoint[joint];
        jointAngles.reserve(jointSamples.size());

        Angles current;
        for (size_t sampleIndex = 0; sampleIndex < jointSamples.size(); ++sampleIndex)
        {
            if (sampleIndex > 0)
            {
                const double dt = static_cast<double>(
                    jointSamples[sampleIndex]->timestampMs - jointSamples[sampleIndex - 1]->timestampMs) / 1000.0;
                const GyroSample& gyro = *jointSamples[sampleIndex - 1];
                current.x = std::clamp(current.x + static_cast<float>(gyro.x / GyroScale * dt), -20.0f, 120.0f);
                current.y = std::clamp(current.y + static_cast<float>(gyro.y / GyroScale * dt), -120.0f, 120.0f);
                current.z = std::clamp(current.z + static_cast<float>(gyro.z / GyroScale * dt), -80.0f, 80.0f);
            }
            jointAngles.push_back(current);
        }
    }

    std::vector<std::uint64_t> timestamps;
    timestamps.reserve(samples.size());
    for (const GyroSample& sample : samples)
    {
        if (timestamps.empty() || timestamps.back() != sample.timestampMs)
            timestamps.push_back(sample.timestampMs);
    }

    for (const std::uint64_t timestamp : timestamps)
    {
        Frame frame;
        frame.time = static_cast<float>(timestamp) / 1000.0f;
        frame.joints.resize(JointCount);

        std::array<Angles, JointCount> anglesAtTime;
        for (int joint = 0; joint < JointCount; ++joint)
        {
            const auto& jointSamples = samplesByJoint[joint];
            const auto& jointAngles = anglesByJoint[joint];
            const auto sample = std::upper_bound(
                jointSamples.begin(), jointSamples.end(), timestamp,
                [](std::uint64_t value, const GyroSample* sample) {
                    return value < sample->timestampMs;
                });
            if (sample != jointSamples.begin())
                anglesAtTime[joint] = jointAngles[static_cast<size_t>(sample - jointSamples.begin() - 1)];
        }

        BuildJoints(frame.joints, anglesAtTime);
        m_track.Add(frame);
    }
    m_track.SortByTime();
}

void CharlesArm::BuildJoints(std::vector<Vector3>& joints, const std::array<Angles, JointCount>& angles)
{
    constexpr float upperArmLength = 0.30f;
    constexpr float forearmLength = 0.25f;
    constexpr float handLength = 0.10f;

    const auto buildSide = [&](int shoulder, int elbow, int wrist, int hand, float restZ) {
        const float shoulderElevation = angles[shoulder].x * DEG2RAD;
        const float shoulderLateral = angles[shoulder].z * DEG2RAD;
        joints[shoulder] = {angles[shoulder].z * 0.01f, 1.5f + angles[shoulder].x * 0.01f, restZ};
        joints[elbow] = {
            joints[shoulder].x + upperArmLength * std::sin(shoulderElevation),
            joints[shoulder].y - upperArmLength * std::cos(shoulderElevation),
            joints[shoulder].z + upperArmLength * std::sin(shoulderLateral)};

        const float elbowElevation = (angles[shoulder].x + angles[elbow].x) * DEG2RAD;
        const float elbowLateral = (angles[shoulder].z + angles[elbow].z) * DEG2RAD;
        joints[wrist] = {
            joints[elbow].x + forearmLength * std::sin(elbowElevation),
            joints[elbow].y - forearmLength * std::cos(elbowElevation),
            joints[elbow].z + forearmLength * std::sin(elbowLateral)};

        const float wristElevation =
            (angles[shoulder].x + angles[elbow].x + angles[wrist].x) * DEG2RAD;
        joints[hand] = {
            joints[wrist].x + handLength * std::sin(wristElevation),
            joints[wrist].y - handLength * std::cos(wristElevation),
            joints[wrist].z};
    };

    buildSide(LeftShoulder, LeftElbow, LeftWrist, LeftHand, -0.20f);
    buildSide(RightShoulder, RightElbow, RightWrist, RightHand, 0.20f);
}

void CharlesArm::DrawArm(int shoulder, int elbow, int wrist, int hand) const
{
    const Vector3& shoulderPosition = m_current.joints[shoulder];
    const Vector3& elbowPosition = m_current.joints[elbow];
    const Vector3& wristPosition = m_current.joints[wrist];
    const Vector3& handPosition = m_current.joints[hand];

    Scene::DrawBoneShadow(shoulderPosition, elbowPosition);
    Scene::DrawBoneShadow(elbowPosition, wristPosition);
    Scene::DrawBoneShadow(wristPosition, handPosition);
    Scene::DrawJointShadow(shoulderPosition);
    Scene::DrawJointShadow(elbowPosition);
    Scene::DrawJointShadow(wristPosition);
    Scene::DrawJointShadow(handPosition);
    Scene::DrawBone(shoulderPosition, elbowPosition);
    Scene::DrawBone(elbowPosition, wristPosition);
    Scene::DrawBone(wristPosition, handPosition);
    Scene::DrawJoint(shoulderPosition, JointColor(shoulder));
    Scene::DrawJoint(elbowPosition, JointColor(elbow));
    Scene::DrawJoint(wristPosition, JointColor(wrist));
    Scene::DrawJoint(handPosition, JointColor(hand));
}
