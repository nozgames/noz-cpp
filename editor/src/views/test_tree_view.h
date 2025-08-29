//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once
#include "tree_view.h"

class TestTreeView : public TreeView
{
public:
    TestTreeView();
    
    // Override to allow popping from stack
    bool CanPopFromStack() const override { return true; }
    
private:
    void PopulateTestData();
};