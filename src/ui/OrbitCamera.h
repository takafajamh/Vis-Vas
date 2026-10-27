#pragma once

#include <imgui.h>
#include <raylib.h>
#include <raymath.h>

#include <algorithm>
#include <cmath>

/*
Camera that circles around a target point, like in Blender or a 3D model viewer.

 Instead of storing the camera position directly, we store:
   target   - the point we look at (e.g. the speaker's chest)
   distance - how far the camera is from the target
   yaw      - angle around the target, left/right (degrees)
   pitch    - angle above/below the target (degrees)
 and calculate the position from those every frame. That makes rotating = changing two angles.

 Controls: Left-drag = rotate, Right-drag = pan, Wheel = zoom,
           R = reset, 1 = front, 2 = side, 3 = top
*/

class OrbitCamera
{
public:
    OrbitCamera(Vector3 target, float distance, float yawDeg, float pitchDeg)
        : m_defaultTarget(target), m_defaultDistance(distance), m_defaultYaw(yawDeg), m_defaultPitch(pitchDeg)
    {
        m_camera.up = { 0.0f, 1.0f, 0.0f };
        m_camera.fovy = 45.0f;
        m_camera.projection = CAMERA_PERSPECTIVE;
        Reset();
    }

    // Call once per frame, before BeginMode3D(). Reads mouse/keyboard and moves the camera.
    void Update()
    {
        const ImGuiIO& io = ImGui::GetIO();

        // Only start dragging when the click is NOT on an ImGui window (e.g. the timeline bar),
        // but keep dragging even if the mouse later moves over one.
        if (!io.WantCaptureMouse)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))  m_rotating = true;
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) m_panning = true;

            Zoom(GetMouseWheelMove());
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))  m_rotating = false;
        if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) m_panning = false;

        const Vector2 delta = GetMouseDelta(); // how many pixels the mouse moved since last frame
        if (m_rotating)
            Rotate(delta);
        if (m_panning)
            Pan(delta);

        if (!io.WantCaptureKeyboard)
        {
            if (IsKeyPressed(KEY_R)) Reset();
            if (IsKeyPressed(KEY_ONE))   SetView(0.0f, 0.0f);   // front
            if (IsKeyPressed(KEY_TWO))   SetView(90.0f, 0.0f);  // side
            if (IsKeyPressed(KEY_THREE)) SetView(0.0f, 89.0f);  // top (not exactly 90, see Rotate)
        }

        UpdatePosition();
    }

    // The raylib camera to pass to BeginMode3D().
    const Camera3D& Get() const { return m_camera; }

    // Back to the starting view.
    void Reset()
    {
        m_target = m_defaultTarget;
        m_distance = m_defaultDistance;
        m_yaw = m_defaultYaw;
        m_pitch = m_defaultPitch;
        UpdatePosition();
    }

    // Jump to a fixed angle, keeping target and zoom.
    void SetView(float yawDeg, float pitchDeg)
    {
        m_yaw = yawDeg;
        m_pitch = pitchDeg;
        UpdatePosition();
    }

private:
    void Rotate(Vector2 mouseDelta)
    {
        constexpr float degreesPerPixel = 0.3f;
        m_yaw -= mouseDelta.x * degreesPerPixel;
        m_pitch += mouseDelta.y * degreesPerPixel;

        // Stop just before straight up/down, otherwise the camera flips over.
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    }

    void Zoom(float wheel)
    {
        // Each wheel step moves 10% closer/further, so zooming feels the same near and far.
        m_distance *= 1.0f - wheel * 0.1f;
        m_distance = std::clamp(m_distance, 0.2f, 20.0f);
    }

    void Pan(Vector2 mouseDelta)
    {
        // Move the target along the screen's left/right and up/down directions.
        const Vector3 forward = Vector3Normalize(Vector3Subtract(m_camera.target, m_camera.position));
        const Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, m_camera.up));
        const Vector3 up = Vector3CrossProduct(right, forward);

        // Further away = move more per pixel, so the scene follows the mouse at any zoom.
        const float scale = m_distance * 0.0015f;
        m_target = Vector3Add(m_target, Vector3Scale(right, -mouseDelta.x * scale));
        m_target = Vector3Add(m_target, Vector3Scale(up, mouseDelta.y * scale));
    }

    // Turn (target, distance, yaw, pitch) into a camera position on a sphere around the target.
    void UpdatePosition()
    {
        const float yaw = m_yaw * DEG2RAD;
        const float pitch = m_pitch * DEG2RAD;

        const Vector3 offset = {
            m_distance * std::cos(pitch) * std::sin(yaw),
            m_distance * std::sin(pitch),
            m_distance * std::cos(pitch) * std::cos(yaw),
        };
        m_camera.target = m_target;
        m_camera.position = Vector3Add(m_target, offset);
    }

    Camera3D m_camera = {};

    Vector3 m_target;
    float m_distance;
    float m_yaw;
    float m_pitch;

    const Vector3 m_defaultTarget;
    const float m_defaultDistance;
    const float m_defaultYaw;
    const float m_defaultPitch;

    bool m_rotating = false;
    bool m_panning = false;
};
