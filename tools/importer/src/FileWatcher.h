/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>

namespace noz::import
{
	class FileWatcher
	{
	public:
		using ChangeCallback = std::function<void(const std::string& path, bool isDirectory)>;

		FileWatcher();
		~FileWatcher();

		// Start watching a directory
		bool watch(const std::string& path, ChangeCallback callback);

		// Stop watching
		void stop();

		// Check if currently watching
		bool isWatching() const { return _watching; }

	private:
		void watchThread();

		std::string _watchPath;
		ChangeCallback _callback;
		std::thread _thread;
		std::atomic<bool> _watching;
		std::atomic<bool> _shouldStop;

#ifdef _WIN32
		void* _directoryHandle; // HANDLE
#endif
	};
}