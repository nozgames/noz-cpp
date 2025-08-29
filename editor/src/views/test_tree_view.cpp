//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "test_tree_view.h"

TestTreeView::TestTreeView()
{
    PopulateTestData();
}

void TestTreeView::PopulateTestData()
{
    // Add some sample hierarchical data to test the tree view using semantic objects and properties
    BeginObject("Game Systems");
    
        BeginObject("Player System");
        
            BeginObject("Movement Component");
                AddProperty("Velocity", "vec3(1.2, 0.0, -0.5)");
                AddProperty("Acceleration", "vec3(0.0, -9.81, 0.0)");
            EndObject();
            
            BeginObject("Input Component");
                AddProperty("Active Actions", "[MOVE_LEFT, AIM]");
                AddProperty("Mouse Position", "(512, 384)");
                AddProperty("Color", "#668811");
            EndObject();
            
            BeginObject("Bow Component");
                AddProperty("Charge Level", "0.75");
                AddProperty("Max Charge", "1.0");
            EndObject();
            
        EndObject();
        
        BeginObject("Sheep Management");
            AddProperty("Active Sheep Count", "12");
            
            BeginObject("White Sheep");
                AddProperty("Sheep #1 - Position", "(15.2, 3.1, 0.0)");
                AddProperty("Sheep #2 - Position", "(8.7, 2.9, 0.0)");
                AddProperty("Sheep #3 - Position", "(-4.1, 3.2, 0.0)");
            EndObject();
            
            BeginObject("Black Sheep");
                AddProperty("Sheep #4 - Position", "(22.1, 3.0, 0.0)");
            EndObject();
            
            BeginObject("Golden Sheep");
                AddProperty("Sheep #5 - Position", "(-12.3, 3.1, 0.0)");
                AddProperty("Shell Intact", "true");
            EndObject();
            
        EndObject();
        
        BeginObject("Wolf AI");
            AddProperty("State", "TARGETING");
            AddProperty("Target", "Sheep #2");
            AddProperty("Speed Multiplier", "1.3");
            AddProperty("Balloons Attached", "0");
        EndObject();
        
    EndObject();
    
    BeginObject("Rendering");
    
        BeginObject("Camera");
            AddProperty("Position", "(0.0, 15.0, 10.0)");
            AddProperty("Target", "(0.0, 0.0, 0.0)");
            AddProperty("FOV", "45.0 degrees");
        EndObject();
        
        BeginObject("Render Queue");
        
            BeginObject("Background Layer");
                AddProperty("Sky Gradient", "3 vertices");
                AddProperty("Clouds", "5 sprites");
            EndObject();
            
            BeginObject("Game Layer");
                AddProperty("Sheep", "12 sprites");
                AddProperty("Wolf", "1 sprite");
                AddProperty("Player", "1 sprite");
                AddProperty("Arrows", "3 sprites");
            EndObject();
            
            BeginObject("UI Layer");
                AddProperty("Score", "\"Wool: 247\"");
                AddProperty("Lives", "3 hearts");
            EndObject();
            
        EndObject();
        
    EndObject();
    
    BeginObject("Audio");
    
        BeginObject("Background Music");
            AddProperty("File", "\"pastoral_theme.ogg\"");
            AddProperty("Volume", "0.6");
            AddProperty("Loop", "true");
        EndObject();
        
        BeginObject("Sound Effects");
            AddProperty("Arrow Shot", "\"bow_release.wav\"");
            AddProperty("Balloon Pop", "\"pop.wav\"");
            AddProperty("Sheep Bounce", "\"bounce.wav\"");
            AddProperty("Wool Collect", "\"collect.wav\"");
        EndObject();
        
    EndObject();
    
    BeginObject("Debug Info");
        AddProperty("Frame Rate", "60.0 FPS");
        AddProperty("Frame Time", "16.67ms");
        
        BeginObject("Memory Usage");
            AddProperty("Total Allocated", "15.2 MB");
            AddProperty("Textures", "8.1 MB");
            AddProperty("Audio", "2.3 MB");
            AddProperty("Meshes", "1.8 MB");
            AddProperty("Other", "3.0 MB");
        EndObject();
        
    EndObject();
    
    BeginObject("Performance Metrics");
        AddProperty("Update Time", "2.1ms");
        AddProperty("Render Time", "12.8ms");
        AddProperty("Collision Checks", "156");
        AddProperty("Entities Active", "23");
    EndObject();
    
    SetCursorVisible(true);
}