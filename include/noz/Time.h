/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz
{
    class Time
    {
    public:

		static float fixedTime() { return _fixedTime; }
        static float deltaTime() { return _deltaTime; }
        static float totalTime() { return _totalTime; }
        
        static void update();
        
        static void initialize();

    private:
        
		static float _deltaTime;
        static float _totalTime;
		static float _fixedTime;

		static std::chrono::high_resolution_clock::time_point _lastFrameTime;
        static std::chrono::high_resolution_clock::time_point _startTime;
    };

}
