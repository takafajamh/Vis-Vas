#pragma once

#include "Playback.h"
#include "ui/Theme.h"

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include <algorithm>

/*
Shared drawing helpers for the 3D view, so every skeleton (fake arm, Charles vest, ...)
looks the same. Objects call DrawBone/DrawJoint in Render(); main calls the rest.
*/

namespace Scene
{
    // Vertical gradient behind everything. Call before BeginMode3D().
    inline void DrawBackground()
    {
        DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), Theme::BackgroundTop, Theme::BackgroundBottom);
    }

    // Moves the picture so the centre of the 3D view lands in the middle of the free area
    // (right of the side panel, above the timeline) instead of the middle of the window.
    // Call right after BeginMode3D(). It shifts the image on screen, it does not move the camera.
    inline void CenterViewIn(float left, float top, float right, float bottom)
    {
        const float width = static_cast<float>(GetScreenWidth());
        const float height = static_cast<float>(GetScreenHeight());

        // Screen pixels -> how far to move the image, in OpenGL's -1..1 screen units.
        const float shiftX = (left + right) / width - 1.0f;
        const float shiftY = 1.0f - (top + bottom) / height;

        // New projection = old projection, then move the result sideways/up.
        const Matrix projection = MatrixMultiply(rlGetMatrixProjection(), MatrixTranslate(shiftX, shiftY, 0.0f));
        rlMatrixMode(RL_PROJECTION);
        rlLoadIdentity();
        rlMultMatrixf(MatrixToFloat(projection));
        rlMatrixMode(RL_MODELVIEW);
    }

    // Floor grid on y = 0 with brighter centre lines. Replaces raylib's DrawGrid (which has fixed colours).
    inline void DrawFloorGrid(int halfLines = 10, float spacing = 0.25f)
    {
        const float extent = halfLines * spacing;
        for (int i = -halfLines; i <= halfLines; ++i)
        {
            const float p = i * spacing;
            const Color color = i == 0 ? Theme::GridAxis : Theme::GridLine;
            DrawLine3D({ p, 0.0f, -extent }, { p, 0.0f, extent }, color);
            DrawLine3D({ -extent, 0.0f, p }, { extent, 0.0f, p }, color);
        }
    }

    // A bone as a rounded tube instead of a 1-pixel line.
    inline void DrawBone(Vector3 from, Vector3 to, float radius = 0.018f)
    {
        DrawCapsule(from, to, radius, 8, 4, Theme::Bone);
    }

    // A joint as a coloured ball.
    inline void DrawJoint(Vector3 position, Color color, float radius = 0.03f)
    {
        DrawSphere(position, radius, color);
    }

    // Floor shadows: show how far forward/back/sideways the body is without rotating the camera.
    // Drawn just above the grid so the grid doesn't hide them.

    // Dark disc on the floor under a joint.
    inline void DrawJointShadow(Vector3 position, float radius = 0.04f)
    {
        DrawCylinder({ position.x, 0.002f, position.z }, radius, radius, 0.001f, 20, Theme::Shadow);
    }

    // Dark band on the floor under a bone.
    inline void DrawBoneShadow(Vector3 from, Vector3 to)
    {
        DrawCylinderEx({ from.x, 0.003f, from.z }, { to.x, 0.003f, to.z }, 0.015f, 0.015f, 6, Theme::Shadow);
    }
}
