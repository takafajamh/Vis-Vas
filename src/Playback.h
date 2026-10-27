#pragma once

#include <raylib.h>
#include <raymath.h>

#include <algorithm>
#include <vector>

/*
Shared playback contract between the data side and the UI side.

 Timeline     - the ONE clock of the program. UI reads/changes it, everything else only reads it.
 Frame        - where every joint is at one moment in time.
 FrameTrack   - a list of Frames over the whole recording + "give me the frame at time t".
 TimedObject  - anything that can be shown at a given time t (Charles vest, fake arm, mat, ...).

 Every frame, Program does:
   timeline.Update(realDt);              // move the clock (if playing)
   obj->SetTime(timeline.GetTime());     // tell every object what time it is
   obj->Render();                        // draw it
*/

// ------------------------------------ Timeline ------------------------------------------

class Timeline
{
public:
    // Called once per rendered frame with the real time that passed (raylib: GetFrameTime()).
    // Moves the clock forward by realDt * speed, so speed 2 = double speed, -1 = play backwards.
    void Update(float realDt)
    {
        if (!m_playing)
            return;

        m_time += realDt * m_speed;

        // Stop at the ends instead of running past the recording.
        if (m_time >= m_duration || m_time <= 0.0f)
        {
            m_time = std::clamp(m_time, 0.0f, m_duration);
            m_playing = false;
        }
    }

    void Play()       { m_playing = true; }
    void Pause()      { m_playing = false; }
    void TogglePlay() { m_playing = !m_playing; }
    bool IsPlaying() const { return m_playing; }

    // Jump straight to a time, e.g. when the user drags the timeline slider.
    void Seek(float t) { m_time = std::clamp(t, 0.0f, m_duration); }

    // Back to the start (the "rewind" button).
    void Rewind() { Seek(0.0f); }

    void SetSpeed(float speed) { m_speed = speed; }
    float GetSpeed() const { return m_speed; }

    // Length of the recording in seconds. Set it after loading data (longest object wins).
    void SetDuration(float duration) { m_duration = std::max(0.0f, duration); Seek(m_time); }
    float GetDuration() const { return m_duration; }

    // The current playback time in seconds. This is the "t" passed to SetTime().
    float GetTime() const { return m_time; }

private:
    float m_time = 0.0f;
    float m_duration = 0.0f;
    float m_speed = 1.0f;
    bool m_playing = false;
};

// ------------------------------------ Frames ------------------------------------------

// One snapshot: the time it was recorded and the 3D position of every joint.
// Joint order is decided by the object (e.g. 0 = shoulder, 1 = elbow, 2 = wrist).
struct Frame
{
    float time = 0.0f;
    std::vector<Vector3> joints;
};

// All precomputed Frames of one recording, sorted by time.
// The data side fills it once after loading (PreCompute), the render side only calls Sample(t).
class FrameTrack
{
public:
    void Add(const Frame& frame) { m_frames.push_back(frame); }

    // Call once after all Add() calls, in case the CSV rows were not in time order.
    void SortByTime()
    {
        std::sort(m_frames.begin(), m_frames.end(),
                  [](const Frame& a, const Frame& b) { return a.time < b.time; });
    }

    bool Empty() const { return m_frames.empty(); }

    float GetDuration() const { return m_frames.empty() ? 0.0f : m_frames.back().time; }

    // Returns the joint positions at time t.
    // Sensors record at fixed moments (e.g. every 10 ms), but t can be anything (e.g. 1.234 s),
    // so we find the two recorded frames around t and blend between them (linear interpolation).
    Frame Sample(float t) const
    {
        if (m_frames.empty())
            return {};
        if (t <= m_frames.front().time)
            return m_frames.front();
        if (t >= m_frames.back().time)
            return m_frames.back();

        // Binary search: first frame whose time is greater than t.
        auto after = std::upper_bound(m_frames.begin(), m_frames.end(), t,
                                      [](float value, const Frame& f) { return value < f.time; });
        const Frame& next = *after;
        const Frame& prev = *(after - 1);

        // How far t is between prev and next: 0 = at prev, 1 = at next.
        const float span = next.time - prev.time;
        const float alpha = span > 0.0f ? (t - prev.time) / span : 0.0f;

        Frame result;
        result.time = t;
        const size_t count = std::min(prev.joints.size(), next.joints.size());
        result.joints.reserve(count);
        for (size_t i = 0; i < count; ++i)
            result.joints.push_back(Vector3Lerp(prev.joints[i], next.joints[i], alpha));
        return result;
    }

private:
    std::vector<Frame> m_frames;
};

// ------------------------------------ Contract ------------------------------------------

// Anything that can be shown at a given point in time.
// Replaces Update(dt): the object no longer keeps its own clock, it is TOLD the time.
class TimedObject
{
public:
    virtual ~TimedObject() = default;

    // "Where should you be at time t?" - look up / compute your state for t. No drawing here.
    virtual void SetTime(float t) = 0;

    // Draw the state chosen by the last SetTime(). Called inside BeginMode3D/EndMode3D.
    virtual void Render() = 0;

    // How long this object's data lasts in seconds, so the Timeline knows its length.
    virtual float GetDuration() const = 0;
};
