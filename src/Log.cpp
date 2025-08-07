/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz
{
	void Log::info(const std::string& group, const std::string& message)
	{
		std::cout << "[" << group << "] " << message << std::endl;
	}

	void Log::warning(const std::string& group, const std::string& message)
	{
		std::cout << "[" << group << "] warning: " << message << std::endl;
	}

	void Log::error(const std::string& group, const std::string& message)
	{
		std::cout << "[" << group << "] error: " << message << std::endl;
	}

	void Log::error(const std::string& group, const std::string& filename, const std::string& message)
	{
		std::cout << "[" << group << "] error: " << filename << ": " << message << std::endl;
	}

}
