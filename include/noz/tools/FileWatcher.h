/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <filesystem>
#include <queue>
#include <mutex>
#include <chrono>
#include <iostream>
#include <efsw/efsw.hpp>

namespace noz::tools
{
	class FileWatcher : public efsw::FileWatchListener
	{
	public:
		using ChangeCallback = std::function<void()>;
		using DirectoryChangeCallback = std::function<void(const std::string&)>;

		FileWatcher();
		~FileWatcher();
		
		// Make the class non-copyable but moveable
		FileWatcher(const FileWatcher&) = delete;
		FileWatcher& operator=(const FileWatcher&) = delete;
		FileWatcher(FileWatcher&& other) noexcept;
		FileWatcher& operator=(FileWatcher&& other) noexcept;

		// Start watching a single file (legacy interface)
		bool watchFile(const std::string& filePath, ChangeCallback callback);
		
		// Start watching a directory for changes
		bool watchDirectory(const std::string& directoryPath, DirectoryChangeCallback callback);

		// Stop watching
		void stop();

		// Check if currently watching
		bool isWatching() const { return _watching; }

		// Get the file/directory being watched
		const std::string& watchedPath() const { return _watchedPath; }
		
		// Process queued callbacks on main thread
		void update();

		// efsw::FileWatchListener implementation
		void handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename) override;

	private:
		// Queue callback for main thread execution
		void queueCallback(std::function<void()> callback);

		std::string _watchedPath;
		std::string _watchedFile; // For single file watching compatibility
		ChangeCallback _callback;
		DirectoryChangeCallback _directoryCallback;
		std::atomic<bool> _watching;
		bool _watchingDirectory;
		
		// efsw objects
		efsw::FileWatcher* _efsw;
		efsw::WatchID _watchID;
		
		// Main thread callback queue with delay support
		struct DelayedCallback {
			std::function<void()> callback;
			std::chrono::steady_clock::time_point executeTime;
		};
		std::queue<DelayedCallback> _callbackQueue;
		std::mutex _callbackMutex;
	};
}