/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/ui/TextEngine.h>
#include <noz/EventManager.h>
#include <noz/Event.h>

namespace noz
{
    using noz::Resources;

    Application::Application() 
        : _window(nullptr)
        , _isRunning(false)
        , _hasFocus(true)
        , _focusHandler(nullptr)
        , _vsyncEnabled(true)
        , _resizeHandler(nullptr)
        , _screenWidth(1920)
        , _screenHeight(1080)
    {
    }

    Application::~Application()
    {
        // OnUnload will be called by ISingleton::Unload()
    }

	Application* Application::load(int width, int height, const std::string& title)
	{
		ISingleton<Application>::load();
		instance()->loadInternal(width, height, title);
		return instance();
	}

	void Application::loadInternal(int width, int height, const std::string& title)
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD) != 1)
			return;

		// Get window settings from config (with fallbacks)
		Uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

		// Try to get window settings from config if available
		// Note: We can't access Config directly from here due to library separation
		// The window settings will be applied when the config is loaded in main.cpp

		_window = SDL_CreateWindow(title.c_str(), width, height, windowFlags);
		if (!_window)
		{
			SDL_Quit();
			return;
		}

		// Initialize cached screen dimensions
		updateScreenDimensions();

		noz::Time::initialize();

		ISingleton<Resources>::load();
		EventManager::load();
		ui::TextEngine::load();
		InputSystem::load();
		renderer::Renderer::load(_window);
	}

    bool Application::update()
    {
        // Update the time system at the start of each frame
        noz::Time::update();
        
        // Process queued events
        Event::update();
        
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                _isRunning = false;
				return false;
            }

            // Handle window focus events
            if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED)
            {
                bool previousFocus = _hasFocus;
                _hasFocus = true;
                if (!previousFocus && _focusHandler)
                {
                    _focusHandler(true);
                }
            }
            else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
            {
                bool previousFocus = _hasFocus;
                _hasFocus = false;
                if (previousFocus && _focusHandler)
                {
                    _focusHandler(false);
                }
            }

            // Handle window resize events
            if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                // Update cached screen dimensions
                updateScreenDimensions();
                               
				Event::send(ScreenSizeChanged{ _screenWidth, _screenHeight });

                // Call custom resize handler if set
                if (_resizeHandler)
                    _resizeHandler(_screenWidth, _screenHeight);
            }

            // Process input events through the InputSystem
            if (InputSystem::instance())
            {
                InputSystem::instance()->ProcessEvents(event);
            }
        }

		return true;
    }

    noz::renderer::CommandBuffer* Application::beginRender()
    {
		if (noz::renderer::Renderer::instance() == nullptr)
			return nullptr;

		return noz::renderer::Renderer::instance()->beginFrame();
    }
    
    void Application::endRender()
    {
        // New CommandBuffer system executes all recorded commands
		auto renderer = noz::renderer::Renderer::instance();
        if (renderer)
        {
            renderer->endFrame();
        }
    }    


    void Application::setScreenSize(int width, int height)
    {
        if (_window)
        {
            SDL_SetWindowSize(_window, width, height);
            updateScreenDimensions();
        }
    }

    void Application::setScreenTitle(const std::string& title)
    {
        if (_window)
        {
            SDL_SetWindowTitle(_window, title.c_str());
        }
    }

    void Application::setFullscreen(bool fullscreen)
    {
        if (_window)
        {
            if (fullscreen)
            {
                SDL_SetWindowFullscreen(_window, SDL_WINDOW_FULLSCREEN);
            }
            else
            {
                SDL_SetWindowFullscreen(_window, 0);
            }
            updateScreenDimensions();
        }
    }

    void Application::setVSync(bool vsync)
    {
        // VSync is typically handled by the renderer, not the window
        // This could be implemented by setting a flag that the renderer checks
        // For now, we'll just store the setting
        _vsyncEnabled = vsync;
    }

    void Application::setResizeHandler(std::function<void(int width, int height)> handler)
    {
        _resizeHandler = handler;
    }

    void Application::updateScreenDimensions()
    {
        if (_window)
        {
            SDL_GetWindowSize(_window, &_screenWidth, &_screenHeight);
        }
    }

    float Application::screenAspectRatio() const
    {
        if (_screenHeight <= 0)
            return 16.0f / 9.0f; // Avoid division by zero
            
        return static_cast<float>(_screenWidth) / static_cast<float>(_screenHeight);
    }

	void Application::unload()
	{
		instance()->unloadInternal();
		ISingleton<Application>::unload();
	}
	
	void Application::unloadInternal()
	{
        InputSystem::unload();
        EventManager::unload();
        ISingleton<Resources>::unload();
        noz::renderer::Renderer::unload();

		if (_window)
        {
            SDL_DestroyWindow(_window);
            _window = nullptr;
        }

        SDL_Quit();
    }

    void Application::setFocusHandler(FocusEventHandler handler)
    {
        _focusHandler = handler;
    }
}
