#pragma once

#include "Playback.h"

#include <imgui.h>
#include <raylib.h>

#include <cstdio>

/*
Bottom bar of the window (see wireframe): playback buttons, time slider and speed.
It only controls the Timeline - it never touches the 3D objects directly.

 Shortcuts: Space = play/pause, Left/Right = jump 1 second, Home = back to start
*/

class TimelineUI
{
public:
    static constexpr float Height = 80.0f; // pixels, so the 3D view can leave room for it

    explicit TimelineUI(Timeline& timeline) : m_timeline(timeline) {}

    void Render()
    {
        HandleShortcuts();

        // Stick the bar to the bottom of the window, full width (follows window resizes).
        const float screenWidth = static_cast<float>(GetScreenWidth());
        const float screenHeight = static_cast<float>(GetScreenHeight());
        ImGui::SetNextWindowPos({ 0.0f, screenHeight - Height });
        ImGui::SetNextWindowSize({ screenWidth, Height });

        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;
        ImGui::Begin("Timeline", nullptr, flags);

        RenderButtons();
        ImGui::SameLine();
        RenderTimeText();
        ImGui::SameLine();
        RenderSpeed();

        RenderSlider();

        ImGui::End();
    }

private:
    // Row 1, left: |<  -1s  Play/Pause  +1s
    void RenderButtons()
    {
        if (ImGui::Button("|<"))
            m_timeline.Rewind();

        ImGui::SameLine();
        if (ImGui::Button("-1s"))
            Jump(-1.0f);

        ImGui::SameLine();
        // Fixed width so the buttons next to it don't shift when the text changes.
        // Width is measured from the longer word, so it fits any font size or padding.
        const float playWidth = ImGui::CalcTextSize("Pause").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        if (ImGui::Button(m_timeline.IsPlaying() ? "Pause" : "Play", { playWidth, 0.0f }))
            TogglePlay();

        ImGui::SameLine();
        if (ImGui::Button("+1s"))
            Jump(1.0f);
    }

    // Row 1, middle: "00:03.25 / 00:10.00"
    void RenderTimeText()
    {
        char current[16];
        char total[16];
        FormatTime(m_timeline.GetTime(), current);
        FormatTime(m_timeline.GetDuration(), total);
        ImGui::Text("%s / %s", current, total);
    }

    // Row 1, right: speed dropdown
    void RenderSpeed()
    {
        static constexpr float speeds[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };

        char label[16];
        std::snprintf(label, sizeof(label), "%gx", m_timeline.GetSpeed());

        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::BeginCombo("Speed", label))
        {
            for (float speed : speeds)
            {
                std::snprintf(label, sizeof(label), "%gx", speed);
                if (ImGui::Selectable(label, speed == m_timeline.GetSpeed()))
                    m_timeline.SetSpeed(speed);
            }
            ImGui::EndCombo();
        }
    }

    // Row 2: full-width slider to scrub through the recording
    void RenderSlider()
    {
        float time = m_timeline.GetTime();
        ImGui::SetNextItemWidth(-FLT_MIN); // fill the remaining width
        if (ImGui::SliderFloat("##time", &time, 0.0f, m_timeline.GetDuration(), ""))
            m_timeline.Seek(time);

        // Pause while the user drags the slider, continue playing when they let go,
        // so playback doesn't fight with the mouse.
        if (ImGui::IsItemActivated())
        {
            m_resumeAfterScrub = m_timeline.IsPlaying();
            m_timeline.Pause();
        }
        if (ImGui::IsItemDeactivated() && m_resumeAfterScrub)
            m_timeline.Play();
    }

    void HandleShortcuts()
    {
        // Don't steal keys while the user types into an ImGui text field.
        if (ImGui::GetIO().WantCaptureKeyboard)
            return;

        if (IsKeyPressed(KEY_SPACE))
            TogglePlay();
        if (IsKeyPressed(KEY_LEFT))
            Jump(-1.0f);
        if (IsKeyPressed(KEY_RIGHT))
            Jump(1.0f);
        if (IsKeyPressed(KEY_HOME))
            m_timeline.Rewind();
    }

    // Pressing Play at the end starts over from the beginning, like a video player.
    void TogglePlay()
    {
        if (!m_timeline.IsPlaying() && m_timeline.GetTime() >= m_timeline.GetDuration())
            m_timeline.Rewind();
        m_timeline.TogglePlay();
    }

    void Jump(float seconds) { m_timeline.Seek(m_timeline.GetTime() + seconds); }

    // seconds -> "mm:ss.cc"
    static void FormatTime(float seconds, char (&out)[16])
    {
        const int minutes = static_cast<int>(seconds) / 60;
        std::snprintf(out, sizeof(out), "%02d:%05.2f", minutes, seconds - minutes * 60.0f);
    }

    Timeline& m_timeline;
    bool m_resumeAfterScrub = false;
};
