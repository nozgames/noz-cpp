//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once
#include "inspector_view.h"
#include <memory>

class TestTreeView : public InspectorView
{
public:
    TestTreeView();
    
    // Override to allow popping from stack
    bool CanPopFromStack() const override { return true; }
    
private:
    void PopulateTestData();
    std::unique_ptr<InspectorObject> _test_data;  // Store the test data
};