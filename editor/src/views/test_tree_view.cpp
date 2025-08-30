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
    // Create a proper InspectorObject hierarchy for testing
    auto root = std::make_unique<InspectorObject>("Game Systems");
    
    // Player System
    auto player_system = root->AddChild("Player System");
    
    // Movement Component  
    auto movement_component = player_system->AddChild("Movement Component");
    movement_component->AddProperty("Velocity", "vec3(1.2, 0.0, -0.5)");
    movement_component->AddProperty("Acceleration", "vec3(0.0, -9.81, 0.0)");
    
    // Input Component
    auto input_component = player_system->AddChild("Input Component");
    input_component->AddProperty("Active Actions", "[MOVE_LEFT, AIM]");
    input_component->AddProperty("Mouse Position", "(512, 384)");
    input_component->AddProperty("Color", "#668811");
    
    // Bow Component
    auto bow_component = player_system->AddChild("Bow Component");
    bow_component->AddProperty("Charge Level", "0.75");
    bow_component->AddProperty("Max Charge", "1.0");
    
    // Sheep Management
    auto sheep_mgmt = root->AddChild("Sheep Management");
    sheep_mgmt->AddProperty("Active Sheep Count", "12");
    
    auto white_sheep = sheep_mgmt->AddChild("White Sheep");
    white_sheep->AddProperty("Sheep #1 - Position", "(15.2, 3.1, 0.0)");
    white_sheep->AddProperty("Sheep #2 - Position", "(8.7, 2.9, 0.0)");
    white_sheep->AddProperty("Sheep #3 - Position", "(-4.1, 3.2, 0.0)");
    
    auto black_sheep = sheep_mgmt->AddChild("Black Sheep");
    black_sheep->AddProperty("Sheep #4 - Position", "(22.1, 3.0, 0.0)");
    
    auto golden_sheep = sheep_mgmt->AddChild("Golden Sheep");
    golden_sheep->AddProperty("Sheep #5 - Position", "(-12.3, 3.1, 0.0)");
    golden_sheep->AddProperty("Shell Intact", "true");
    
    // Wolf AI
    auto wolf_ai = root->AddChild("Wolf AI");
    wolf_ai->AddProperty("State", "TARGETING");
    wolf_ai->AddProperty("Target", "Sheep #2");
    wolf_ai->AddProperty("Speed Multiplier", "1.3");
    wolf_ai->AddProperty("Balloons Attached", "0");
    
    // Rendering
    auto rendering = root->AddChild("Rendering");
    
    auto camera = rendering->AddChild("Camera");
    camera->AddProperty("Position", "(0.0, 15.0, 10.0)");
    camera->AddProperty("Target", "(0.0, 0.0, 0.0)");
    camera->AddProperty("FOV", "45.0 degrees");
    
    auto render_queue = rendering->AddChild("Render Queue");
    
    auto bg_layer = render_queue->AddChild("Background Layer");
    bg_layer->AddProperty("Sky Gradient", "3 vertices");
    bg_layer->AddProperty("Clouds", "5 sprites");
    
    auto game_layer = render_queue->AddChild("Game Layer");
    game_layer->AddProperty("Sheep", "12 sprites");
    game_layer->AddProperty("Wolf", "1 sprite");
    game_layer->AddProperty("Player", "1 sprite");
    game_layer->AddProperty("Arrows", "3 sprites");
    
    auto ui_layer = render_queue->AddChild("UI Layer");
    ui_layer->AddProperty("Score", "\"Wool: 247\"");
    ui_layer->AddProperty("Lives", "3 hearts");
    
    // Store the root object and build the tree
    _test_data = std::move(root);
    SetRootObject(_test_data.get());
    
    SetCursorVisible(true);
}