#pragma once

#include <imgui.h>
#include <raylib.h>

/*
All colours of the app in one place, so the 3D view and the ImGui panels match.
Accent = teal, taken from the presentation slides.
*/

namespace Theme
{
    // 3D view
    inline constexpr Color BackgroundTop    = { 40, 44, 52, 255 };
    inline constexpr Color BackgroundBottom = { 16, 18, 22, 255 };
    inline constexpr Color GridLine         = { 58, 64, 74, 255 };
    inline constexpr Color GridAxis         = { 84, 92, 106, 255 };
    inline constexpr Color Bone             = { 205, 210, 220, 255 };
    inline constexpr Color Shadow           = { 60, 170, 180, 70 }; // soft teal "projection" - black is invisible on a dark floor

    // Accent (teal)
    inline constexpr ImVec4 Accent       = { 0.13f, 0.60f, 0.64f, 1.00f };
    inline constexpr ImVec4 AccentHover  = { 0.18f, 0.70f, 0.74f, 1.00f };
    inline constexpr ImVec4 AccentActive = { 0.10f, 0.50f, 0.54f, 1.00f };

    // Dark ImGui style with rounded corners and the teal accent. Call once after rlImGuiSetup().
    inline void ApplyImGuiStyle()
    {
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding = 0.0f; // panels are docked to the window edges
        style.FrameRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.PopupRounding = 4.0f;
        style.WindowBorderSize = 0.0f;
        style.FramePadding = { 8.0f, 4.0f };
        style.ItemSpacing = { 8.0f, 6.0f };
        style.WindowPadding = { 10.0f, 10.0f };

        ImVec4* c = style.Colors;
        c[ImGuiCol_WindowBg]         = { 0.08f, 0.09f, 0.11f, 0.97f };
        c[ImGuiCol_PopupBg]          = { 0.10f, 0.11f, 0.13f, 0.98f };
        c[ImGuiCol_FrameBg]          = { 0.16f, 0.18f, 0.21f, 1.00f };
        c[ImGuiCol_FrameBgHovered]   = { 0.20f, 0.23f, 0.27f, 1.00f };
        c[ImGuiCol_FrameBgActive]    = { 0.24f, 0.27f, 0.32f, 1.00f };

        c[ImGuiCol_Button]           = { 0.16f, 0.18f, 0.21f, 1.00f };
        c[ImGuiCol_ButtonHovered]    = AccentHover;
        c[ImGuiCol_ButtonActive]     = AccentActive;

        c[ImGuiCol_Header]           = { 0.13f, 0.60f, 0.64f, 0.35f };
        c[ImGuiCol_HeaderHovered]    = { 0.13f, 0.60f, 0.64f, 0.55f };
        c[ImGuiCol_HeaderActive]     = { 0.13f, 0.60f, 0.64f, 0.75f };

        c[ImGuiCol_CheckMark]        = AccentHover;
        c[ImGuiCol_SliderGrab]       = Accent;
        c[ImGuiCol_SliderGrabActive] = AccentHover;
        c[ImGuiCol_Separator]        = { 0.22f, 0.25f, 0.29f, 1.00f };
        c[ImGuiCol_TextDisabled]     = { 0.50f, 0.54f, 0.60f, 1.00f };
    }
}
