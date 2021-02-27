# ==============================================================================
# LLVM Release License
# ==============================================================================
# University of Illinois/NCSA
# Open Source License
#
# Copyright (c) 2003-2018 University of Illinois at Urbana-Champaign.
# All rights reserved.
#
# Developed by:
#
# LLVM Team
#
# University of Illinois at Urbana-Champaign
#
# http://llvm.org
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal with
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is furnished to do
# so, subject to the following conditions:
#
# * Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimers.
#
# * Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimers in the
# documentation and/or other materials provided with the distribution.
#
# * Neither the names of the LLVM Team, University of Illinois at
# Urbana-Champaign, nor the names of its contributors may be used to
# endorse or promote products derived from this Software without specific
# prior written permission.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
# SOFTWARE.

# MODIFIED 02/27/21 to support checking for 128-bit atomics

# outputs:
# NEED_ATOMICS  - normal atomics
# NEED_ATOMICS_FOR_64 	- atomics with 64-bit variables
# NEED_ATOMICS_FOR_128 		- atomics with 128-bit variables

INCLUDE(CheckCXXSourceCompiles)
#INCLUDE(CheckLibraryExists)

# Sometimes linking against libatomic is required for atomic ops, if
# the platform doesn't support lock-free atomics.

function(check_working_cxx_atomics varname)
	set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
	set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -std=c++11")
	CHECK_CXX_SOURCE_COMPILES("
		#include <atomic>
		std::atomic<int> x;
		int main()
		{
			return std::atomic_is_lock_free(&x);
		}
		" ${varname}
	)
	set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})
endfunction(check_working_cxx_atomics)

function(check_working_cxx_atomics64 varname)
	set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
	set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -std=c++11")
	CHECK_CXX_SOURCE_COMPILES("
		#include <atomic>
		#include <cstdint>
		std::atomic<uint64_t> x (0);
		int main()
		{
			uint64_t i = x.load(std::memory_order_relaxed);
			return std::atomic_is_lock_free(&x);
		}
		" ${varname}
	)
	set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})
endfunction(check_working_cxx_atomics64)

# test to resolve [undefined reference to `__atomic_load_16'] and related
function(check_working_cxx_atomics128 varname)
	set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
	set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -std=c++11")
	CHECK_CXX_SOURCE_COMPILES("
		#include <atomic>
		#include <cstdint>

		struct Atomic128
		{
			std::uint64_t a{1};
			std::uint64_t b{2};
		};

		int main()
		{
			std::atomic<Atomic128> x;
			uint64_t i = x.load(std::memory_order_relaxed).a;

			return std::atomic_is_lock_free(&x);
		}
		" ${varname}
	)
	set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})
endfunction(check_working_cxx_atomics128)


# This isn't necessary on MSVC, so avoid command-line switch annoyance
# by only running on GCC-like hosts.
if (LLVM_COMPILER_IS_GCC_COMPATIBLE)
	# First check if atomics work without the library.
	check_working_cxx_atomics(HAVE_CXX_ATOMICS_WITHOUT_LIB)
	# If not check if atomics work with it (will also fail if libatomic not present)
	# note: previous version of this file tested
	#  check_library_exists(atomic __atomic_fetch_add_4 "" HAVE_LIBATOMIC)
	# however it threw errors if language 'C' was not specified
	#TODO: test if 'C' is available first, then call check_library_exists, or fixme
	if(NOT HAVE_CXX_ATOMICS_WITHOUT_LIB)
		list(APPEND CMAKE_REQUIRED_LIBRARIES "atomic")
		check_working_cxx_atomics(HAVE_CXX_ATOMICS_WITH_LIB)
		if (NOT HAVE_CXX_ATOMICS_WITH_LIB)
			message(WARNING "Host compiler does not support std::atomic!")
		endif()
	endif()
endif()

# set output variable
set(NEED_ATOMICS ${HAVE_CXX_ATOMICS_WITHOUT_LIB})

# Check for 64 bit atomic operations.
if(MSVC)
	set(HAVE_CXX_ATOMICS64_WITHOUT_LIB True)
else()
	check_working_cxx_atomics64(HAVE_CXX_ATOMICS64_WITHOUT_LIB)
endif()

# If not, check if the library exists, and atomics work with it.
if(NOT HAVE_CXX_ATOMICS64_WITHOUT_LIB)
	list(APPEND CMAKE_REQUIRED_LIBRARIES "atomic")
	check_working_cxx_atomics64(HAVE_CXX_ATOMICS64_WITH_LIB)
	if (NOT HAVE_CXX_ATOMICS64_WITH_LIB)
		message(WARNING "Host compiler does not support std::atomic!")
	endif()
endif()

set(NEED_ATOMICS_FOR_64 ${HAVE_CXX_ATOMICS64_WITHOUT_LIB})

# Check 128-bit atomics
# first see if 128-bit atomics are available without 'atomic'
check_working_cxx_atomics128(HAVE_CXX_ATOMICS128_WITHOUT_LIB)

# then if they aren't, see if adding 'atomic' helps
if(NOT HAVE_CXX_ATOMICS128_WITHOUT_LIB)
	list(APPEND CMAKE_REQUIRED_LIBRARIES "atomic")
	check_working_cxx_atomics128(HAVE_CXX_ATOMICS128_WITH_LIB)
	if (NOT HAVE_CXX_ATOMICS128_WITH_LIB)
		message(WARNING "Host compiler does not support std::atomic!")
	endif()
endif()

set(NEED_ATOMICS_FOR_128 ${HAVE_CXX_ATOMICS128_WITHOUT_LIB})