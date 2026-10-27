#pragma once

#include "Playback.h"
#include "ui/Scene.h"

#include <cmath>

/*
Test object with made-up motion, so the UI (timeline, camera, trails) can be built
before the real CSV parser exists. Swap it for the real Charles object later -
both implement TimedObject, so Program does not need to change.

It also shows the idea the real object should follow:
 1. PreCompute: sensor orientations + bone lengths -> joint positions (forward kinematics), stored in a FrameTrack
 2. SetTime:    look up the frame for t
 3. Render:     draw joints as spheres, bones as lines
*/

class FakeArm : public TimedObject
{
public:
    // Joint indices inside Frame::joints.
    enum Joint { Shoulder = 0, Elbow = 1, Wrist = 2, JointCount = 3 };

    explicit FakeArm(float durationSeconds = 10.0f, float sampleRateHz = 100.0f)
    {
        PreCompute(durationSeconds, sampleRateHz);
    }

    void SetTime(float t) override
    {
        m_current = m_track.Sample(t);
    }

    void Render() override
    {
        if (m_current.joints.size() < JointCount)
            return;

        const Vector3& shoulder = m_current.joints[Shoulder];
        const Vector3& elbow    = m_current.joints[Elbow];
        const Vector3& wrist    = m_current.joints[Wrist];

        // Shadows on the floor first, so the arm is drawn over them
        Scene::DrawBoneShadow(shoulder, elbow);
        Scene::DrawBoneShadow(elbow, wrist);
        Scene::DrawJointShadow(shoulder);
        Scene::DrawJointShadow(elbow);
        Scene::DrawJointShadow(wrist);

        // Bones
        Scene::DrawBone(shoulder, elbow);
        Scene::DrawBone(elbow, wrist);

        // Joints
        Scene::DrawJoint(shoulder, JointColor(Shoulder));
        Scene::DrawJoint(elbow, JointColor(Elbow));
        Scene::DrawJoint(wrist, JointColor(Wrist));
    }

    float GetDuration() const override { return m_track.GetDuration(); }

    // Current joint positions.
    const Frame& GetCurrentFrame() const { return m_current; }

    // All precomputed frames, e.g. for drawing movement trails.
    const FrameTrack& GetTrack() const { return m_track; }

    static const char* JointName(int joint)
    {
        static const char* names[JointCount] = { "Shoulder", "Elbow", "Wrist" };
        return names[joint];
    }

    static Color JointColor(int joint)
    {
        static const Color colors[JointCount] = { RED, ORANGE, GREEN };
        return colors[joint];
    }

private:
    void PreCompute(float durationSeconds, float sampleRateHz)
    {
        const Vector3 shoulderPos = { 0.2f, 1.5f, 0.0f }; // fixed point on the vest, in metres
        const float upperArmLength = 0.30f;
        const float forearmLength  = 0.25f;
        const Vector3 boneDirection = { 0.0f, -1.0f, 0.0f }; // a bone with no rotation hangs straight down

        const int sampleCount = static_cast<int>(durationSeconds * sampleRateHz);
        for (int i = 0; i <= sampleCount; ++i)
        {
            const float t = i / sampleRateHz;

            // Fake "sensor" orientations. The real object reads these from the CSV instead.
            // Upper arm swings forward/back and a bit sideways, forearm bends at the elbow.
            const Quaternion upperArmRot = QuaternionFromEuler(0.8f * std::sin(t * 1.5f),       // pitch: swing forward/back
                                                               0.0f,                            // yaw
                                                               0.3f + 0.2f * std::sin(t * 0.7f)); // roll: lift sideways
            const Quaternion elbowBend   = QuaternionFromEuler(0.6f + 0.5f * std::sin(t * 2.0f), 0.0f, 0.0f);

            // The forearm's world rotation = upper arm rotation, then the elbow bend on top of it.
            const Quaternion forearmRot = QuaternionMultiply(upperArmRot, elbowBend);

            // Forward kinematics: start at the shoulder, walk along each bone in the direction it points.
            Frame frame;
            frame.time = t;
            frame.joints.resize(JointCount);
            frame.joints[Shoulder] = shoulderPos;
            frame.joints[Elbow]    = Vector3Add(frame.joints[Shoulder],
                                                Vector3Scale(Vector3RotateByQuaternion(boneDirection, upperArmRot), upperArmLength));
            frame.joints[Wrist]    = Vector3Add(frame.joints[Elbow],
                                                Vector3Scale(Vector3RotateByQuaternion(boneDirection, forearmRot), forearmLength));

            m_track.Add(frame);
        }
        m_track.SortByTime();
    }

    FrameTrack m_track;
    Frame m_current;
};
