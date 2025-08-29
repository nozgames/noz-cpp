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
    // Add some sample hierarchical data to test the tree view
    AddLine("Game Systems");
    AddLine("\tPlayer System");
    AddLine("\t\tMovement Component");
    AddLine("\t\t\tVelocity: vec3(1.2, 0.0, -0.5)");
    AddLine("\t\t\tAcceleration: vec3(0.0, -9.81, 0.0)");
    AddLine("\t\tInput Component");
    AddLine("\t\t\tActive Actions: [MOVE_LEFT, AIM]");
    AddLine("\t\t\tMouse Position: (512, 384)");
    AddLine("\t\tBow Component");
    AddLine("\t\t\tCharge Level: 0.75");
    AddLine("\t\t\tMax Charge: 1.0");
    AddLine("\tSheep Management");
    AddLine("\t\tActive Sheep Count: 12");
    AddLine("\t\tWhite Sheep");
    AddLine("\t\t\tSheep #1 - Position: (15.2, 3.1, 0.0)");
    AddLine("\t\t\tSheep #2 - Position: (8.7, 2.9, 0.0)");
    AddLine("\t\t\tSheep #3 - Position: (-4.1, 3.2, 0.0)");
    AddLine("\t\tBlack Sheep");
    AddLine("\t\t\tSheep #4 - Position: (22.1, 3.0, 0.0)");
    AddLine("\t\tGolden Sheep");
    AddLine("\t\t\tSheep #5 - Position: (-12.3, 3.1, 0.0)");
    AddLine("\t\t\tShell Intact: true");
    AddLine("\tWolf AI");
    AddLine("\t\tState: TARGETING");
    AddLine("\t\tTarget: Sheep #2");
    AddLine("\t\tSpeed Multiplier: 1.3");
    AddLine("\t\tBalloons Attached: 0");
    AddLine("Rendering");
    AddLine("\tCamera");
    AddLine("\t\tPosition: (0.0, 15.0, 10.0)");
    AddLine("\t\tTarget: (0.0, 0.0, 0.0)");
    AddLine("\t\tFOV: 45.0 degrees");
    AddLine("\tRender Queue");
    AddLine("\t\tBackground Layer");
    AddLine("\t\t\tSky Gradient: 3 vertices");
    AddLine("\t\t\tClouds: 5 sprites");
    AddLine("\t\tGame Layer");
    AddLine("\t\t\tSheep: 12 sprites");
    AddLine("\t\t\tWolf: 1 sprite");
    AddLine("\t\t\tPlayer: 1 sprite");
    AddLine("\t\t\tArrows: 3 sprites");
    AddLine("\t\tUI Layer");
    AddLine("\t\t\tScore: \"Wool: 247\"");
    AddLine("\t\t\tLives: 3 hearts");
    AddLine("Audio");
    AddLine("\tBackground Music: \"pastoral_theme.ogg\"");
    AddLine("\t\tVolume: 0.6");
    AddLine("\t\tLoop: true");
    AddLine("\tSound Effects");
    AddLine("\t\tArrow Shot: \"bow_release.wav\"");
    AddLine("\t\tBalloon Pop: \"pop.wav\"");
    AddLine("\t\tSheep Bounce: \"bounce.wav\"");
    AddLine("\t\tWool Collect: \"collect.wav\"");
    AddLine("Debug Info");
    AddLine("\tFrame Rate: 60.0 FPS");
    AddLine("\tFrame Time: 16.67ms");
    AddLine("\tMemory Usage");
    AddLine("\t\tTotal Allocated: 15.2 MB");
    AddLine("\t\tTextures: 8.1 MB");
    AddLine("\t\tAudio: 2.3 MB");
    AddLine("\t\tMeshes: 1.8 MB");
    AddLine("\t\tOther: 3.0 MB");
    AddLine("Performance Metrics");
    AddLine("\tUpdate Time: 2.1ms");
    AddLine("\tRender Time: 12.8ms");
    AddLine("\tCollision Checks: 156");
    AddLine("\tEntities Active: 23");
    
    // Start with some nodes collapsed for demonstration
    SetCursorVisible(true);
}