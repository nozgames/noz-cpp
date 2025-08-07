/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/tools/FileWatcher.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace noz::tools
{
	FileWatcher::FileWatcher()
		: _watching(false)
		, _shouldStop(false)
		, _watchingDirectory(false)
#ifdef _WIN32
		, _directoryHandle(INVALID_HANDLE_VALUE)
#endif
	{
	}

	FileWatcher::~FileWatcher()
	{
		stop();
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
		_shouldStop = false;

#ifdef _WIN32
		// Extract directory and filename for Windows directory watching
		std::filesystem::path path(filePath);
		_watchDirectory = path.parent_path().string();
		_filename = path.filename().string();

		// Open directory handle
		_directoryHandle = CreateFileA(
			_watchDirectory.c_str(),
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			NULL
		);

		if (_directoryHandle == INVALID_HANDLE_VALUE)
		{
			std::cerr << "Failed to open directory for watching: " << _watchDirectory << std::endl;
			return false;
		}
#endif

		_watching = true;
		_thread = std::thread(&FileWatcher::watchThread, this);
		return true;
	}
	
	bool FileWatcher::watchDirectory(const std::string& directoryPath, DirectoryChangeCallback callback)
	{
		if (_watching)
		{
			std::cerr << "FileWatcher is already watching" << std::endl;
			return false;
		}

		// Validate directory exists
		if (!std::filesystem::exists(directoryPath) || !std::filesystem::is_directory(directoryPath))
		{
			std::cerr << "Directory does not exist: " << directoryPath << std::endl;
			return false;
		}

		_watchedPath = directoryPath;
		_directoryCallback = callback;
		_callback = nullptr;
		_watchingDirectory = true;
		_shouldStop = false;
		
		// Initialize file modification times for polling
		_fileModTimes.clear();
		try
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(directoryPath))
			{
				if (entry.is_regular_file())
				{
					_fileModTimes[entry.path().string()] = entry.last_write_time();
				}
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error initializing directory watch: " << e.what() << std::endl;
			return false;
		}

#ifdef _WIN32
		// For directory watching on Windows, we'll use the existing directory handle approach
		_watchDirectory = directoryPath;

		// Open directory handle
		_directoryHandle = CreateFileA(
			directoryPath.c_str(),
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			NULL
		);

		if (_directoryHandle == INVALID_HANDLE_VALUE)
		{
			std::cerr << "Failed to open directory for watching: " << directoryPath << std::endl;
			return false;
		}
#endif

		_watching = true;
		_thread = std::thread(&FileWatcher::watchThread, this);
		return true;
	}

	void FileWatcher::stop()
	{
		if (!_watching)
			return;

		_shouldStop = true;

#ifdef _WIN32
		if (_directoryHandle != INVALID_HANDLE_VALUE)
		{
			CancelIo(_directoryHandle);
			CloseHandle(_directoryHandle);
			_directoryHandle = INVALID_HANDLE_VALUE;
		}
#endif

		if (_thread.joinable())
		{
			_thread.join();
		}

		_watching = false;
	}

	void FileWatcher::watchThread()
	{
		if (_watchingDirectory)
		{
			// Use polling for directory watching (works on all platforms)
			pollDirectoryChanges();
		}
		else
		{
			// Single file watching (legacy implementation)
#ifdef _WIN32
			const DWORD bufferSize = 64 * 1024; // 64KB
			std::vector<BYTE> buffer(bufferSize);
			
			OVERLAPPED overlapped = {};
			overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

			// Start the first read
			BOOL success = ReadDirectoryChangesW(
				_directoryHandle,
				buffer.data(),
				bufferSize,
				FALSE, // Don't watch subdirectories for single file
				FILE_NOTIFY_CHANGE_LAST_WRITE,
				nullptr,
				&overlapped,
				NULL
			);

			if (!success)
			{
				std::cerr << "Initial ReadDirectoryChangesW failed" << std::endl;
				CloseHandle(overlapped.hEvent);
				return;
			}

			while (!_shouldStop)
			{
				// Wait for changes
				DWORD waitResult = WaitForSingleObject(overlapped.hEvent, 100);

				if (waitResult != WAIT_OBJECT_0)
					continue;

				DWORD transferred;
				if (!GetOverlappedResult(_directoryHandle, &overlapped, &transferred, FALSE))
				{
					std::cerr << "GetOverlappedResult failed" << std::endl;
					break;
				}

				// Process the events
				FILE_NOTIFY_INFORMATION* info = (FILE_NOTIFY_INFORMATION*)buffer.data();
				do
				{
					// Convert to UTF-8
					int nameLength = info->FileNameLength / sizeof(WCHAR);
					int utf8Length = WideCharToMultiByte(CP_UTF8, 0, info->FileName, nameLength, nullptr, 0, nullptr, nullptr);
					std::string fileName(utf8Length, '\0');
					WideCharToMultiByte(CP_UTF8, 0, info->FileName, nameLength, &fileName[0], utf8Length, nullptr, nullptr);

					// Check if this is our watched file
					std::filesystem::path watchedPath(_watchedFile);
					if (fileName == watchedPath.filename().string())
					{
						switch (info->Action)
						{
						case FILE_ACTION_MODIFIED:
							if (_callback)
							{
								queueCallback(_callback);
							}
							break;
						}
					}

					// Move to next entry
					if (info->NextEntryOffset == 0)
						break;
					info = (FILE_NOTIFY_INFORMATION*)((BYTE*)info + info->NextEntryOffset);
				} while (true);

				// Reset for next read
				ResetEvent(overlapped.hEvent);
				
				success = ReadDirectoryChangesW(
					_directoryHandle,
					buffer.data(),
					bufferSize,
					FALSE,
					FILE_NOTIFY_CHANGE_LAST_WRITE,
					nullptr,
					&overlapped,
					NULL
				);

				if (!success)
				{
					std::cerr << "ReadDirectoryChangesW failed during monitoring" << std::endl;
					break;
				}
			}

			CloseHandle(overlapped.hEvent);
#else
			// Fallback polling implementation for non-Windows single file watching
			std::filesystem::file_time_type lastModTime;
			try
			{
				lastModTime = std::filesystem::last_write_time(_watchedFile);
			}
			catch (const std::exception& e)
			{
				std::cerr << "Error getting initial file time: " << e.what() << std::endl;
				return;
			}

			while (!_shouldStop)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(500));

				try
				{
					if (std::filesystem::exists(_watchedFile))
					{
						auto modTime = std::filesystem::last_write_time(_watchedFile);
						if (modTime != lastModTime)
						{
							lastModTime = modTime;
							if (_callback)
							{
								queueCallback(_callback);
							}
						}
					}
				}
				catch (const std::exception& e)
				{
					std::cerr << "Error during file watch: " << e.what() << std::endl;
				}
			}
#endif
		}
	}
	
	void FileWatcher::pollDirectoryChanges()
	{
		while (!_shouldStop)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));

			try
			{
				// Check all files in the directory recursively
				for (const auto& entry : std::filesystem::recursive_directory_iterator(_watchedPath))
				{
					if (entry.is_regular_file())
					{
						std::string filePath = entry.path().string();
						auto modTime = entry.last_write_time();
						
						auto it = _fileModTimes.find(filePath);
						if (it == _fileModTimes.end())
						{
							// New file
							_fileModTimes[filePath] = modTime;
							if (_directoryCallback)
							{
								queueCallback([this, filePath]() { _directoryCallback(filePath); });
							}
						}
						else if (it->second != modTime)
						{
							// Modified file
							it->second = modTime;
							if (_directoryCallback)
							{
								queueCallback([this, filePath]() { _directoryCallback(filePath); });
							}
						}
					}
				}
				
				// Remove files that no longer exist
				auto it = _fileModTimes.begin();
				while (it != _fileModTimes.end())
				{
					if (!std::filesystem::exists(it->first))
					{
						it = _fileModTimes.erase(it);
					}
					else
					{
						++it;
					}
				}
			}
			catch (const std::exception& e)
			{
				std::cerr << "Error during directory watch: " << e.what() << std::endl;
			}
		}
	}

	void FileWatcher::update()
	{
		std::lock_guard<std::mutex> lock(_callbackMutex);
		auto now = std::chrono::steady_clock::now();
		
		while (!_callbackQueue.empty())
		{
			auto& delayedCallback = _callbackQueue.front();
			if (delayedCallback.executeTime <= now)
			{
				delayedCallback.callback();
				_callbackQueue.pop();
			}
			else
			{
				// Still waiting for delay to expire
				break;
			}
		}
	}

void FileWatcher::queueCallback(std::function<void()> callback)
	{
		std::lock_guard<std::mutex> lock(_callbackMutex);
		
		// Add a 100ms delay to allow file system operations to complete
		auto executeTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
		_callbackQueue.push({callback, executeTime});
	}
}