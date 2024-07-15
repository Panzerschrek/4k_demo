cmake_minimum_required(VERSION 3.13.4)

project(4k_demo)

# Setup C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)


if(MSVC)
	add_compile_options(/utf-8 /W3 /WX)
	add_compile_options(/GR-) # no RTTI
else()
	add_compile_options(-Werror -Wall -Wextra -Wconversion)
	add_compile_options(-fvisibility=hidden -fvisibility-inlines-hidden)
	add_compile_options(-fdata-sections -ffunction-sections) # simplify later gc-sections.
	add_compile_options( -fno-rtti )

	# Clear garbage.
	string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " -s -Wl,--gc-sections")
	string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE " -s -Wl,--gc-sections")
endif()

add_executable(4k_demo main.cpp)