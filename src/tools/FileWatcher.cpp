/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/tools/FileWatcher.h>

namespace noz::tools
{
	FileWatcher::FileWatcher()
		: _watching(false)
		, _watchingDirectory(false)
		, _efsw(nullptr)
		, _watchID(-1)
	{
		_efsw = new efsw::FileWatcher();
	}

	FileWatcher::~FileWatcher()
	{
		stop();
		delete _efsw;
	}

	FileWatcher::FileWatcher(FileWatcher&& other) noexcept
		: _watchedPath(std::move(other._watchedPath))
		, _watchedFile(std::move(other._watchedFile))
		, _callback(std::move(other._callback))
		, _directoryCallback(std::move(other._directoryCallback))
		, _watching(other._watching.load())
		, _watchingDirectory(other._watchingDirectory)
		, _efsw(other._efsw)
		, _watchID(other._watchID)
		, _callbackQueue(std::move(other._callbackQueue))
	{
		// Transfer ownership
		other._efsw = nullptr;
		other._watchID = -1;
		other._watching = false;
		other._watchingDirectory = false;
	}

	FileWatcher& FileWatcher::operator=(FileWatcher&& other) noexcept
	{
		if (this != &other)
		{
			// Clean up current state
			stop();
			delete _efsw;

			// Move from other
			_watchedPath = std::move(other._watchedPath);
			_watchedFile = std::move(other._watchedFile);
			_callback = std::move(other._callback);
			_directoryCallback = std::move(other._directoryCallback);
			_watching = other._watching.load();
			_watchingDirectory = other._watchingDirectory;
			_efsw = other._efsw;
			_watchID = other._watchID;
			_callbackQueue = std::move(other._callbackQueue);

			// Transfer ownership
			other._efsw = nullptr;
			other._watchID = -1;
			other._watching = false;
			other._watchingDirectory = false;
		}
		return *this;
	}

	bool FileWatcher::watchFile(const std::string& filePath, ChangeCallback callback)
	{
		if (_watching)
		{
			std::cerr << "FileWatcher is already watching a file" << std::endl;
			return false;
		}

		// Validate file exists
		if (!std::filesystem::exists(filePath))
		{
			std::cerr << "File does not exist: " << filePath << std::endl;
			return false;
		}

		_watchedPath = filePath;
		_watchedFile = filePath;
		_callback = callback;
		_directoryCallback = nullptr;
		_watchingDirectory = false;

		// Watch the directory containing the file
		std::filesystem::path path(filePath);
		std::string directory = path.parent_path().string();
		
		_watchID = _efsw->addWatch(directory, this, false);
		if (_watchID < 0)
		{
			std::cerr << "Failed to watch directory: " << directory << std::endl;
			return false;
		}

		_efsw->watch();
		_watching = true;
		return true;
	}

	bool FileWatcher::watchDirectory(const std::string& directoryPath, DirectoryChangeCallback callback)
	{
		if (_watching)
		{
			std::cerr << "FileWatcher is already watching a directory" << std::endl;
			return false;
		}

		// Validate directory exists
		if (!std::filesystem::exists(directoryPath) || !std::filesystem::is_directory(directoryPath))
		{
			std::cerr << "Directory does not exist: " << directoryPath << std::endl;
			return false;
		}

		_watchedPath = directoryPath;
		_watchedFile.clear();
		_callback = nullptr;
		_directoryCallback = callback;
		_watchingDirectory = true;

		_watchID = _efsw->addWatch(directoryPath, this, true); // Recursive watching
		if (_watchID < 0)
		{
			std::cerr << "Failed to watch directory: " << directoryPath << std::endl;
			return false;
		}

		_efsw->watch();
		_watching = true;
		return true;
	}

	void FileWatcher::stop()
	{
		if (!_watching)
			return;

		if (_watchID >= 0)
		{
			_efsw->removeWatch(_watchID);
			_watchID = -1;
		}

		_watching = false;
		_watchingDirectory = false;
		_watchedPath.clear();
		_watchedFile.clear();
		_callback = nullptr;
		_directoryCallback = nullptr;

		// Clear pending callbacks
		std::lock_guard<std::mutex> lock(_callbackMutex);
		while (!_callbackQueue.empty())
		{
			_callbackQueue.pop();
		}
	}

	void FileWatcher::handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename)
	{
		// Only process relevant actions
		if (action != efsw::Actions::Add && 
			action != efsw::Actions::Modified && 
			action != efsw::Actions::Moved)
		{
			return;
		}

		// Build full path
		std::string fullPath = dir;
		if (!fullPath.empty() && fullPath.back() != '/' && fullPath.back() != '\\')
		{
			fullPath += "/";
		}
		fullPath += filename;

		// Normalize path separators
		std::replace(fullPath.begin(), fullPath.end(), '\\', '/');

		// Filter out unwanted files
		if (filename.find(".meta") != std::string::npos ||
			filename.find(".TMP") != std::string::npos ||
			filename.find("~") != std::string::npos ||
			filename.empty())
		{
			return;
		}

		// Check if it's a directory
		bool isDirectory = std::filesystem::is_directory(fullPath);

		if (_watchingDirectory)
		{
			// Directory watching mode
			if (_directoryCallback && !isDirectory)
			{
				queueCallback([this, fullPath]() {
					_directoryCallback(fullPath);
				});
			}
		}
		else
		{
			// Single file watching mode
			if (_callback && fullPath == _watchedFile)
			{
				queueCallback([this]() {
					_callback();
				});
			}
		}
	}

	void FileWatcher::queueCallback(std::function<void()> callback)
	{
		std::lock_guard<std::mutex> lock(_callbackMutex);
		DelayedCallback delayed;
		delayed.callback = callback;
		delayed.executeTime = std::chrono::steady_clock::now(); // Execute immediately
		_callbackQueue.push(delayed);
	}

	void FileWatcher::update()
	{
		std::lock_guard<std::mutex> lock(_callbackMutex);
		auto now = std::chrono::steady_clock::now();

		while (!_callbackQueue.empty() && _callbackQueue.front().executeTime <= now)
		{
			_callbackQueue.front().callback();
			_callbackQueue.pop();
		}
	}
}