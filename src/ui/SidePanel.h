#pragma once

#include "Playback.h"
#include "ui/OrbitCamera.h"
#include "ui/TimelineUI.h"
#include "ui/Trails.h"

#include <imgui.h>
#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

/*
Left panel of the window.
 Recording - what is loaded (later: "Load file" button)
 Trails    - turn movement trails on/off, length, which joints
 Camera    - view preset buttons + list of mouse/keyboard controls
*/

class SidePanel
{
public:
    static constexpr float Width = 300.0f; // pixels

    SidePanel(const Timeline& timeline, TrailSettings& trails, OrbitCamera& camera, std::vector<std::string> jointNames)
        : m_timeline(timeline), m_trails(trails), m_camera(camera), m_jointNames(std::move(jointNames))
    {
    }

    void Render()
    {
        // Full height on the left, stopping above the timeline bar.
        ImGui::SetNextWindowPos({ 0.0f, 0.0f });
        ImGui::SetNextWindowSize({ Width, static_cast<float>(GetScreenHeight()) - TimelineUI::Height });

        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("Side Panel", nullptr, flags);

        RenderRecording();
        RenderTrails();
        RenderCamera();

        ImGui::End();
    }

private:
    void RenderRecording()
    {
        if (!ImGui::CollapsingHeader("Recording", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        ImGui::TextWrapped("Source: Fake arm (test data)");
        ImGui::Text("Length: %.2f s", m_timeline.GetDuration());
        ImGui::Spacing();
    }

    void RenderTrails()
    {
        if (!ImGui::CollapsingHeader("Trails", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        ImGui::Checkbox("Show trails", &m_trails.enabled);

        // Grey out the options while trails are off.
        ImGui::BeginDisabled(!m_trails.enabled);

        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::SliderFloat("##length", &m_trails.lengthSeconds, 0.2f, 5.0f, "Length: %.1f s");

        for (size_t joint = 0; joint < m_jointNames.size() && joint < m_trails.showJoint.size(); ++joint)
        {
            // ImGui needs a bool*, and std::vector<bool> can't give one, so copy in and out.
            bool show = m_trails.showJoint[joint];
            if (ImGui::Checkbox(m_jointNames[joint].c_str(), &show))
                m_trails.showJoint[joint] = show;
        }

        ImGui::EndDisabled();
        ImGui::Spacing();
    }

    void RenderCamera()
    {
        if (!ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        if (ImGui::Button("Front")) m_camera.SetView(0.0f, 0.0f);
        ImGui::SameLine();
        if (ImGui::Button("Side")) m_camera.SetView(90.0f, 0.0f);
        ImGui::SameLine();
        if (ImGui::Button("Top")) m_camera.SetView(0.0f, 89.0f);
        ImGui::SameLine();
        if (ImGui::Button("Reset")) m_camera.Reset();

        ImGui::Spacing();
        RenderHelp();
    }

    // "Help" accordion with the mouse/keyboard controls. ImGui opens sections instantly,
    // so the animation is done by hand: the area's height grows/shrinks and the text fades.
    void RenderHelp()
    {
        static constexpr const char* lines[] = {
            "Left-drag  rotate",
            "Right-drag pan",
            "Wheel      zoom",
            "1/2/3      views",
            "R          reset view",
            "Space      play/pause",
            "Left/Right jump 1 s",
        };
        constexpr int lineCount = sizeof(lines) / sizeof(lines[0]);

        // Arrow header; NoTreePushOnOpen = no indent and no TreePop() needed.
        const bool open = ImGui::TreeNodeEx("Help", ImGuiTreeNodeFlags_NoTreePushOnOpen);

        // Move m_helpAnim towards 1 (open) or 0 (closed). Exponential = fast start, soft finish.
        constexpr float speed = 12.0f;
        const float target = open ? 1.0f : 0.0f;
        m_helpAnim += (target - m_helpAnim) * std::min(1.0f, GetFrameTime() * speed);
        if (std::abs(target - m_helpAnim) < 0.001f)
            m_helpAnim = target;

        if (m_helpAnim <= 0.0f)
            return; // fully closed - draw nothing

        // Reveal only part of the text area (clipped by the child window) and fade it in.
        const float fullHeight = lineCount * ImGui::GetTextLineHeightWithSpacing();
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * m_helpAnim);
        ImGui::BeginChild("##help", { 0.0f, fullHeight * m_helpAnim }, ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        for (const char* line : lines)
            ImGui::TextDisabled("%s", line);
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }

    const Timeline& m_timeline;
    TrailSettings& m_trails;
    OrbitCamera& m_camera;
    std::vector<std::string> m_jointNames;

    float m_helpAnim = 0.0f; // 0 = help closed, 1 = fully open
};
