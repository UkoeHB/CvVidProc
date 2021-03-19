// exception-based assert

#ifndef EXCEPTION_ASSERT_8557848_H
#define EXCEPTION_ASSERT_8557848_H

//local headers

//third party headers

//standard headers
#include <string>


/// wrapper around main exception assert
void exception_assert(std::string expr, std::string func, std::string file, int line);

/// throws exception with info about failed exception
void exception_assert(std::string expr, std::string func, std::string file, int line, std::string msg);

#define EXCEPTION_ASSERT_MSG(expression, msg) { if (!(expression)) exception_assert(#expression, __func__, __FILE__, __LINE__, msg);}
#define EXCEPTION_ASSERT(expression) EXCEPTION_ASSERT_MSG(expression, "")



#endif //header guard
