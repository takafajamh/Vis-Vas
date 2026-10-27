#pragma once

#include "CsvGyroLoader.h"
#include "Playback.h"

#include <raylib.h>

#include <array>
#include <vector>

class CharlesArm : public TimedObject
{
public:
    enum Joint
    {
        LeftHand,
        RightHand,
        LeftWrist,
        RightWrist,
        LeftElbow,
        RightElbow,
        LeftShoulder,
        RightShoulder,
        JointCount
    };

    explicit CharlesArm(const GyroRecording& recording);

    void SetTime(float time) override;
    void Render() override;
    float GetDuration() const override;

    const FrameTrack& GetTrack() const { return m_track; }

    static const char* JointName(int joint);
    static Color JointColor(int joint);

private:
    struct Angles
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    void PreCompute(const GyroRecording& recording);
    void DrawArm(int shoulder, int elbow, int wrist, int hand) const;
    static void BuildJoints(std::vector<Vector3>& joints, const std::array<Angles, JointCount>& angles);

    FrameTrack m_track;
    Frame m_current;
};
