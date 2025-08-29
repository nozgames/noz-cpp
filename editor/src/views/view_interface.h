//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

class IView
{
public:
    virtual ~IView() = default;
    virtual void Render(int width, int height) = 0;
    virtual bool HandleKey(int key) = 0;  // Returns true if key was handled
    virtual void SetCursorVisible(bool visible) = 0;
    virtual bool CanPopFromStack() const { return true; }  // Log view should return false
};