// string utility functions

//local headers
#include "string_utils.h"

//third party headers

//standard headers
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <string>
#include <sstream>


std::string format_string(const char* str, ...)
{
	va_list args;
	va_start(args, str);
	std::string ret{format_string_with_args(str, args)};
	va_end(args);

	return ret;
}

std::string format_string_with_args(const char* str, va_list args)
{
	int position = 0;
	int last_position = 0;
	auto length = (int)strlen(str);
	std::string ret{};

	while (true)
	{
		// Scan the format string and find %a argument where a is one of d, f, s ...
		while (position < length && str[position] != '%')
			position++;

		// add part of input string between last format symbol and current format symbol (or begin/end)
		ret.append(str, last_position, position - last_position);

		if (position >= length)
			return ret;

		char format = str[position + 1];
		position += 2;
		last_position = position;
		std::ostringstream ss;

		switch (format)
		{
			// Integer
			case 'd':
			case 'i':
				{
					int arg = va_arg(args, int);
					ss << arg;
					ret += ss.str();
					break;
				}

			// Unsigned
			case 'u':
				{
					unsigned arg = va_arg(args, unsigned);
					ss << arg;
					ret += ss.str();
					break;
				}

			// Unsigned long
			case 'l':
				{
					unsigned long arg = va_arg(args, unsigned long);
					ss << arg;
					ret += ss.str();
					break;
				}

			// Real
			case 'f':
				{
					double arg = va_arg(args, double);
					ss << arg;
					ret += ss.str();
					break;
				}

			// Character
			case 'c':
				{
					int arg = va_arg(args, int);
					ss << static_cast<char>(arg);
					ret += ss.str();
					break;
				}

			// C string
			case 's':
				{
					char* arg = va_arg(args, char*);
					ret += arg;
					break;
				}

			// Hex
			case 'x':
				{
					char buf[CONVERSION_BUFFER_LENGTH];
					int arg = va_arg(args, int);
					int arglen = ::sprintf(buf, "%x", arg);
					ret.append(buf, 0, arglen);
					break;
				}

			// Pointer
			case 'p':
				{
					char buf[CONVERSION_BUFFER_LENGTH];
					int arg = va_arg(args, int);
					int arglen = ::sprintf(buf, "%p", reinterpret_cast<void*>(arg));
					ret.append(buf, 0, arglen);
					break;
				}

			case '%':
				{
					ret += '%';
					break;
				}

			default:
				assert(false && format_string("Unsupported format specifier: '%c'", format).c_str());
				break;
		}
	}
}