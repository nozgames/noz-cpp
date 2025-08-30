//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

class IView
{
public:

    virtual ~IView() = default;
    virtual void Render(const irect_t& rect) = 0;
    virtual bool HandleKey(int key) { return false; }
    virtual void SetCursorVisible(bool visible) {};
    virtual bool CanPopFromStack() const { return true; }
    virtual void SetSearchPattern(const std::string& pattern) {}
    virtual void ClearSearch() {}
    virtual bool SupportsSearch() const { return false; }
};