#include <raylib.h>
#include <rlImGui.h>
#include <imgui.h>

extern "C" 
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <nlohmann/json.hpp>

#include "Playback.h"
#include "FakeArm.h"
#include "ui/TimelineUI.h"
#include "ui/OrbitCamera.h"
#include "ui/SidePanel.h"
#include "ui/Trails.h"
#include "ui/Theme.h"
#include "ui/Scene.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

/*
DataPipeline templated
 - Input - LoadFromCSV(path)
 - Filters - ApplyFilter from DataFilters (AVG, min-max, normalize timestamp to 0)


 Effector objected
 - Input Pipeline
 - Convert to data
 - Update
 - Render


 Program:
 - Updata objects
 - Render game
 - Render UI

 UI:
 - Render()

*/

// ------------------------------------ Data layer ------------------------------------------


struct GyroData
{
    std::string ID;
    std::uint64_t Timestamp;
    double dx, dy, dz;
};

template <typename SensorDataT>
class DataFilter
{
public:
    virtual void ApplyFilter(std::vector<SensorDataT>& data) = 0;                                               // WILL CHANGE THE VECTORS DATA
    std::vector<SensorDataT> ApplyFilterNoOverride(const std::vector<SensorDataT>& data)      // will NOT change the data, TWO COPIES
    {
        std::vector<SensorDataT> returnValue(data);
        ApplyFilter(returnValue);
        return returnValue;
    }
};

template <typename SensorDataT>
class DataPipeline
{
public:
    void AddFilter(const DataFilter<SensorDataT>& filter)
    {
        m_filters.push_back(filter);
    }

    const std::vector<DataFilter<SensorDataT>>& GetFilters() const
    {
        return m_filters;
    }

    bool LoadDataFromCSVFile(const std::string& path)
    {
        // TO IMPLEMENT
        // Load into m_data


        m_path = path;
    }

    void Apply()
    {
        // TO IMPLEMENT
        // if not loaded error
        for (auto& filter : m_filters)
        {
            filter.ApplyFilter(m_data);
        }

        // sort by timestamp

    }


private:
    std::vector<SensorDataT> m_data;
    std::vector<DataFilter<SensorDataT>> m_filters; // applied in order
    std::string m_path;
};

// -------------------------------------- Objects, effectors ----------------------------------------

struct SingleGyroCompute
{
    std::string ID;
    double x, y, z;
};

template <typename SingleSensorComputeT>
class Constraints
{
public:
    virtual void ApplyConstraint(SingleSensorComputeT& data) = 0; // Will override
    
    SingleSensorComputeT ApplyFilterNoOverride(const SingleSensorComputeT& data)      // will NOT change the data, TWO COPIES
    {
       SingleSensorComputeT returnValue = data;
       ApplyConstraint(returnValue);
       return returnValue;
    }
};

class Object
{
public:
    virtual void Update(const float dt) = 0;
    virtual void Render() = 0;
};

class CharlesObject : public Object
{
public:

    CharlesObject(DataPipeline<GyroData>* dataSource)
    {
        m_dataSource = dataSource;
    }

    void AddConstraint(std::unique_ptr<Constraints<SingleGyroCompute>> constraint)
    {
        m_constraints.push_back(std::move(constraint));
    }

    const std::vector<std::unique_ptr<Constraints<SingleGyroCompute>>>& GetConstraints() const
    {
        return m_constraints;
    }

    void Execute()
    {
        ConvertToSensorData();
        PreCompute();
        m_computed = true;
    }


    void Update(const float dt) override
    {
        if (!m_computed)
            return;

        m_currentTime += dt;
        // Change positions like in map 
    }

    void Render() override
    {
        if (!m_computed)
            return;
        // Draw raylib objects to show it
    }

    virtual ~CharlesObject()
    {
        if (m_dataSource != nullptr)
            delete m_dataSource;
    }

private:
    void ConvertToSensorData()
    {
        // Convert Sensor data by sensor into ordered map of ID - vec<SingleGyroCompte>
    }
    void PreCompute()
    {
        // Move
        // Apply Constraints
    }

    bool m_computed = false;
    float m_currentTime = 0;
    std::vector<std::unique_ptr<Constraints<SingleGyroCompute>>> m_constraints;
    DataPipeline<GyroData>* m_dataSource = nullptr;
};

// ------------------------------------ UI ------------------------------------------

class UI
{
public:
    bool Active = true;
    virtual void Render() = 0;
};

// TimelineUI lives in ui/TimelineUI.h

// ----------------------------------- PROGRAM -------------------------------------------
class Program
{
public:
    void Start()
    {
        Configure();
        run();
    }

    ~Program()
    {
        for (auto* obj : m_objects)
            delete obj;
    }

    float dtRate = 0;
    float timelineTime = 0;
private:
    void Configure()
    {
        // Screen size, etc, probably more args in constructor
    }

    void run()
    {
        float dt = 0;

        // main loop + events
            Update(dt);
            Render();
            RenderUI();
    }

    void Update(float dt)
    {
        //...
        if (m_weak_Timeline)
        {
            dtRate = m_weak_Timeline->GetSpeed();
            timelineTime = m_weak_Timeline->GetTime();
        }

    }

    void Render()
    {

    }

    void RenderUI()
    {
        for (auto* ui : m_UIs)
        {
            if (ui->Active)
                ui->Render();
        }
    }

    Timeline* m_weak_Timeline = nullptr; // Do not delete, owned elsewhere
    std::vector<UI*> m_UIs;
    std::vector<Object*> m_objects;

};


int main()
{
    // TODO: move this loop into Program once main.cpp is split into files
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "Vas-Vis");
    SetTargetFPS(60);

    rlImGuiSetup(true); /* dark theme */
    Theme::ApplyImGuiStyle();

    FakeArm arm; // placeholder until the Charles CSV pipeline exists
    Timeline timeline;
    timeline.SetDuration(arm.GetDuration());
    TimelineUI timelineUI(timeline);

    // Look at the arm from front-right and above, far enough back to see the floor shadows under it
    OrbitCamera camera({ 0.2f, 0.7f, 0.0f }, 2.8f, 35.0f, 25.0f);

    // Trails on elbow and wrist by default (the shoulder barely moves)
    TrailSettings trails;
    trails.showJoint = { false, true, true };

    std::vector<std::string> jointNames;
    for (int joint = 0; joint < FakeArm::JointCount; ++joint)
        jointNames.push_back(FakeArm::JointName(joint));
    SidePanel sidePanel(timeline, trails, camera, jointNames);

    while (!WindowShouldClose())
    {
        // 1. Move the clock, 2. tell every object what time it is
        timeline.Update(GetFrameTime());
        arm.SetTime(timeline.GetTime());
        camera.Update();

        BeginDrawing();
        ClearBackground(Theme::BackgroundBottom);
        Scene::DrawBackground();

        // 3. Draw the 3D scene, centred in the area next to the side panel and above the timeline
        BeginMode3D(camera.Get());
        Scene::CenterViewIn(SidePanel::Width, 0.0f, static_cast<float>(GetScreenWidth()),
                            static_cast<float>(GetScreenHeight()) - TimelineUI::Height);
        Scene::DrawFloorGrid();
        DrawTrails(arm.GetTrack(), timeline.GetTime(), trails, FakeArm::JointColor);
        arm.Render();
        EndMode3D();

        // 4. Draw the UI on top
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
