#pragma once

namespace noz
{
	class Log
	{
	public:

		static void info(const std::string& group, const std::string& message);

		static void error(const std::string& group, const std::string& message);

		static void error(const std::string& group, const std::string& filename, const std::string& message);

		static void warning(const std::string& group, const std::string& message);
	};
}
