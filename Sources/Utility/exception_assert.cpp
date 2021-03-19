// exception-based assert

//local headers
#include "exception_assert.h"
#include "project_config.h"
#include "string_utils.h"

//third party headers

//standard headers
#include <stdexcept>
#include <string>


void exception_assert(std::string expr, std::string func, std::string file, int line)
{
	exception_assert(expr, func, file, line, "");
}

void exception_assert(std::string expr, std::string func, std::string file, int line, std::string msg)
{
	const char *buf = "cvvidproc(%s) %s:%d: assert failed in function '%s()'\n%s";
	std::string assert_string{format_string(buf, config::cvvidproc_version, file.c_str(), line, func.c_str(), expr.c_str())};

	if (!msg.empty())
	{
		assert_string += "\nassert msg: %s";
		assert_string = format_string(assert_string.c_str(), msg.c_str());
	}

	throw std::runtime_error(assert_string);
}
