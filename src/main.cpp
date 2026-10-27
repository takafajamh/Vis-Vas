#include <raylib.h>
#include <rlImGui.h>

#include "data/CharlesArm.h"
#include "data/CsvGyroLoader.h"
#include "ui/OrbitCamera.h"
#include "ui/Scene.h"
#include "ui/SidePanel.h"
#include "ui/Theme.h"
#include "ui/TimelineUI.h"
#include "ui/Trails.h"

#include <iostream>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

int main()
{
    GyroRecording recording;
    std::unique_ptr<CharlesArm> arm;
    try
    {
        recording = LoadGyroCsv(std::filesystem::path("data.csv"));
        arm = std::make_unique<CharlesArm>(recording);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Failed to prepare data.csv: " << error.what() << '\n';
        return 1;
    }

    Timeline timeline;
    timeline.SetDuration(arm->GetDuration());
    TimelineUI timelineUI(timeline);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "Vas-Vis");
    SetTargetFPS(60);
    rlImGuiSetup(true);
    Theme::ApplyImGuiStyle();

    OrbitCamera camera({0.2f, 0.7f, 0.0f}, 2.8f, 35.0f, 25.0f);
    TrailSettings trails;
    trails.showJoint = {false, false, true, false, true, false, false, false};

    std::vector<std::string> jointNames;
    for (int joint = 0; joint < CharlesArm::JointCount; ++joint)
        jointNames.emplace_back(CharlesArm::JointName(joint));

    const std::string recordingDescription =
        "data.csv (" + std::to_string(recording.sensorIds.size()) + " sensors, " +
        std::to_string(recording.samples.size()) + " samples)";
    SidePanel sidePanel(timeline, trails, camera, std::move(jointNames), recordingDescription);

    while (!WindowShouldClose())
    {
        timeline.Update(GetFrameTime());
        arm->SetTime(timeline.GetTime());
        camera.Update();

        BeginDrawing();
        ClearBackground(Theme::BackgroundBottom);
        Scene::DrawBackground();

        BeginMode3D(camera.Get());
        Scene::CenterViewIn(SidePanel::Width, 0.0f, static_cast<float>(GetScreenWidth()),
                            static_cast<float>(GetScreenHeight()) - TimelineUI::Height);
        Scene::DrawFloorGrid();
        DrawTrails(arm->GetTrack(), timeline.GetTime(), trails, CharlesArm::JointColor);
        arm->Render();
        EndMode3D();

        rlImGuiBegin();
        sidePanel.Render();
        timelineUI.Render();
        rlImGuiEnd();
        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
    return 0;
}
