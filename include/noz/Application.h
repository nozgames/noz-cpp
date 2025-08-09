/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::renderer
{
	class CommandBuffer;
}

namespace noz
{
    // Focus event callback type
    using FocusEventHandler = std::function<void(bool hasFocus)>;

    class Application : public ISingleton<Application>
    {
    public:
        Application();
        ~Application();

        bool IsRunning() const { return _isRunning; }
        SDL_Window* GetWindow() const { return _window; }
        
        bool update();
                
        void setFocusHandler(FocusEventHandler handler);
        
        // Screen size management (renamed from window size)
        void setScreenSize(int width, int height);
        int screenWidth() const { return _screenWidth; }
        int screenHeight() const { return _screenHeight; }
        void setScreenTitle(const std::string& title);
        void setFullscreen(bool fullscreen);
        void setVSync(bool vsync);
        
        void setResizeHandler(std::function<void(int width, int height)> handler);
        
        // Get screen aspect ratio
        float screenAspectRatio() const;

		noz::renderer::CommandBuffer* beginRender();
		void endRender();

		static std::shared_ptr<Application> load(int width, int height, const std::string& title);
		static void unload();

    private:

        friend class ISingleton<Application>;

		void loadInternal(int width, int height, const std::string& title);
		void unloadInternal();

        SDL_Window* _window;
        bool _isRunning;
        FocusEventHandler _focusHandler;
        bool _hasFocus;
        bool _vsyncEnabled;
        std::function<void(int width, int height)> _resizeHandler;
        
        // Cached screen dimensions
        int _screenWidth;
        int _screenHeight;
        
        // Update cached screen dimensions from SDL
        void updateScreenDimensions();
    };

	struct ScreenSizeChanged
	{
		int width;
		int height;
	};
}