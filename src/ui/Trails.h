#pragma once

#include "Playback.h"

#include <raylib.h>

#include <algorithm>
#include <vector>

/*
Movement trails: a line showing where a joint has been during the last few seconds,
fading out towards the oldest point.

 The trail is NOT recorded while playing. Every frame it is rebuilt from the FrameTrack
 ("where was the wrist between t-1.5s and t?"), so it stays correct when the user
 rewinds, scrubs or changes speed.
*/

// What the side panel changes and the renderer reads.
struct TrailSettings
{
    bool enabled = true;
    float lengthSeconds = 1.5f;
    std::vector<bool> showJoint; // one entry per joint, index = joint index in Frame::joints
};

// Draw the path of one joint from (time - lengthSeconds) up to time. Call inside BeginMode3D().
inline void DrawTrail(const FrameTrack& track, int joint, float time, float lengthSeconds, Color color)
{
    constexpr float pointsPerSecond = 60.0f; // enough points for a smooth curve

    const float start = std::max(0.0f, time - lengthSeconds);
    const int segments = static_cast<int>((time - start) * pointsPerSecond);
    if (segments < 1)
        return;

    Frame previous = track.Sample(start);
    for (int i = 1; i <= segments; ++i)
    {
        const float t = start + (time - start) * i / segments;
        Frame current = track.Sample(t);

        if (static_cast<size_t>(joint) >= current.joints.size() || static_cast<size_t>(joint) >= previous.joints.size())
            return;

        // 0 at the oldest point (invisible, thin) -> 1 at the newest point (solid, thick).
        // Drawn as small tubes instead of 1-pixel lines, so trails are visible on a projector.
        const float fade = static_cast<float>(i) / segments;
        const float previousFade = static_cast<float>(i - 1) / segments;
        constexpr float maxRadius = 0.008f;
        DrawCylinderEx(previous.joints[joint], current.joints[joint], maxRadius * previousFade, maxRadius * fade, 6,
                       ColorAlpha(color, fade));

        previous = std::move(current);
    }
}

// Draw trails for every joint switched on in the settings.
inline void DrawTrails(const FrameTrack& track, float time, const TrailSettings& settings, Color (*jointColor)(int))
{
    if (!settings.enabled)
        return;

    for (size_t joint = 0; joint < settings.showJoint.size(); ++joint)
    {
        if (settings.showJoint[joint])
            DrawTrail(track, static_cast<int>(joint), time, settings.lengthSeconds, jointColor(static_cast<int>(joint)));
    }
}
