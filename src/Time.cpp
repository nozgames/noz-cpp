/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/Time.h>

namespace noz
{
    float Time::_deltaTime = 0.0f;
    float Time::_totalTime = 0.0f;
	float Time::_fixedTime = 1.0f / 50.0f;
    std::chrono::high_resolution_clock::time_point Time::_lastFrameTime;
    std::chrono::high_resolution_clock::time_point Time::_startTime;

    void Time::initialize()
    {
        _startTime = std::chrono::high_resolution_clock::now();
        _lastFrameTime = _startTime;
        _deltaTime = 0.0f;
        _totalTime = 0.0f;
    }

    void Time::update()
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        
        // Calculate delta time
        auto deltaTimeDuration = currentTime - _lastFrameTime;
        _deltaTime = std::chrono::duration<float>(deltaTimeDuration).count();
        
        // Calculate total time
        auto totalTimeDuration = currentTime - _startTime;
        _totalTime = std::chrono::duration<float>(totalTimeDuration).count();
        
        // Update last frame time
        _lastFrameTime = currentTime;
    }
}
