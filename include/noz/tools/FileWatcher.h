/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::tools
{
	class FileWatcher
	{
	public:
		using ChangeCallback = std::function<void()>;
		using DirectoryChangeCallback = std::function<void(const std::string&)>;

		FileWatcher();
		~FileWatcher();

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

	private:
		void watchThread();
		void pollDirectoryChanges();
		
		// Queue callback for main thread execution
		void queueCallback(std::function<void()> callback);

		std::string _watchedPath;
		std::string _watchedFile; // For single file watching compatibility
		ChangeCallback _callback;
		DirectoryChangeCallback _directoryCallback;
		std::thread _thread;
		std::atomic<bool> _watching;
		std::atomic<bool> _shouldStop;
		bool _watchingDirectory;
		
		// For polling-based directory watching
		std::unordered_map<std::string, std::filesystem::file_time_type> _fileModTimes;
		
		// Main thread callback queue with delay support
		struct DelayedCallback {
			std::function<void()> callback;
			std::chrono::steady_clock::time_point executeTime;
		};
		std::queue<DelayedCallback> _callbackQueue;
		std::mutex _callbackMutex;

#ifdef _WIN32
		void* _directoryHandle; // HANDLE
		std::string _watchDirectory;
		std::string _filename;
#endif
	};
}