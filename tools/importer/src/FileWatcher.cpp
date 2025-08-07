/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "FileWatcher.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace noz::import
{
	FileWatcher::FileWatcher()
		: _watching(false)
		, _shouldStop(false)
#ifdef _WIN32
		, _directoryHandle(INVALID_HANDLE_VALUE)
#endif
	{
	}

	FileWatcher::~FileWatcher()
	{
		stop();
	}

	bool FileWatcher::watch(const std::string& path, ChangeCallback callback)
	{
		if (_watching)
		{
			std::cerr << "FileWatcher is already watching a directory" << std::endl;
			return false;
		}

		_watchPath = path;
		_callback = callback;
		_shouldStop = false;

#ifdef _WIN32
		// Open directory handle
		_directoryHandle = CreateFileA(
			path.c_str(),
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			NULL
		);

		if (_directoryHandle == INVALID_HANDLE_VALUE)
		{
			std::cerr << "Failed to open directory for watching: " << path << std::endl;
			return false;
		}
#else
		// For non-Windows platforms, we'll fall back to polling
		std::cout << "Warning: File watching only fully supported on Windows. Using polling fallback." << std::endl;
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
#ifdef _WIN32
		const DWORD bufferSize = 64 * 1024; // 64KB to capture more events
		std::vector<BYTE> buffer1(bufferSize);
		std::vector<BYTE> buffer2(bufferSize);
		std::vector<BYTE>* currentBuffer = &buffer1;
		std::vector<BYTE>* processingBuffer = &buffer2;
		
		OVERLAPPED overlapped = {};
		overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		// Start the first read
		BOOL success = ReadDirectoryChangesW(
			_directoryHandle,
			currentBuffer->data(),
			bufferSize,
			TRUE, // Watch subdirectories
			FILE_NOTIFY_CHANGE_FILE_NAME |
			FILE_NOTIFY_CHANGE_CREATION |
			FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_SIZE |
			FILE_NOTIFY_CHANGE_LAST_WRITE |
			FILE_NOTIFY_CHANGE_LAST_ACCESS |
			FILE_NOTIFY_CHANGE_ATTRIBUTES |
			FILE_NOTIFY_CHANGE_SECURITY,
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

			// Swap buffers - immediately start next read while we process current events
			std::swap(currentBuffer, processingBuffer);
			ResetEvent(overlapped.hEvent);
			
			// Start the next read immediately
			success = ReadDirectoryChangesW(
				_directoryHandle,
				currentBuffer->data(),
				bufferSize,
				TRUE,
				FILE_NOTIFY_CHANGE_FILE_NAME |
				FILE_NOTIFY_CHANGE_CREATION |
				FILE_NOTIFY_CHANGE_DIR_NAME |
				FILE_NOTIFY_CHANGE_SIZE |
				FILE_NOTIFY_CHANGE_LAST_WRITE |
				FILE_NOTIFY_CHANGE_LAST_ACCESS |
				FILE_NOTIFY_CHANGE_ATTRIBUTES |
				FILE_NOTIFY_CHANGE_SECURITY,
				nullptr,
				&overlapped,
				NULL
			);

			if (!success)
			{
				std::cerr << "ReadDirectoryChangesW failed during monitoring" << std::endl;
				break;
			}

			// Now process the events from the buffer we just swapped out
			FILE_NOTIFY_INFORMATION* info = (FILE_NOTIFY_INFORMATION*)processingBuffer->data();
			do
			{
				// Convert to UTF-8
				int nameLength = info->FileNameLength / sizeof(WCHAR);
				int utf8Length = WideCharToMultiByte(CP_UTF8, 0, info->FileName, nameLength, nullptr, 0, nullptr, nullptr);
				std::string fileName(utf8Length, '\0');
				WideCharToMultiByte(CP_UTF8, 0, info->FileName, nameLength, &fileName[0], utf8Length, nullptr, nullptr);

				std::string fullPath = _watchPath + "\\" + fileName;
				std::replace(fullPath.begin(), fullPath.end(), '\\', '/');

				// Debug output for ALL files
				bool isDirectory = (GetFileAttributesA(fullPath.c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0;
				
				std::cout << fullPath << " = " << info->Action << std::endl;

				// Only process non-.meta files for import
				if (fullPath.find(".meta") == std::string::npos &&
					fullPath.find(".TMP") == std::string::npos &&
					!isDirectory &&
					(fullPath[fullPath.size()-1]) != '~')
				{
					switch (info->Action)
					{
					case FILE_ACTION_ADDED:
					case FILE_ACTION_MODIFIED:
					case FILE_ACTION_RENAMED_NEW_NAME:
						_callback(fullPath, isDirectory);
						break;
					}
				}

				// Move to next entry
				if (info->NextEntryOffset == 0)
					break;
				info = (FILE_NOTIFY_INFORMATION*)((BYTE*)info + info->NextEntryOffset);
			} while (true);
		}

		CloseHandle(overlapped.hEvent);
#else
		// Fallback polling implementation for non-Windows
		std::unordered_map<std::string, std::filesystem::file_time_type> fileModTimes;

		// Initialize modification times
		for (const auto& entry : std::filesystem::recursive_directory_iterator(_watchPath))
		{
			if (entry.is_regular_file() && entry.path().extension() != ".meta")
			{
				fileModTimes[entry.path().string()] = entry.last_write_time();
			}
		}

		while (!_shouldStop)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));

			try
			{
				for (const auto& entry : std::filesystem::recursive_directory_iterator(_watchPath))
				{
					if (!entry.is_regular_file() || entry.path().extension() == ".meta")
						continue;

					std::string filePath = entry.path().string();
					auto lastWriteTime = entry.last_write_time();

					auto it = fileModTimes.find(filePath);
					if (it == fileModTimes.end() || it->second != lastWriteTime)
					{
						fileModTimes[filePath] = lastWriteTime;
						_callback(filePath, false);
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