cmake_minimum_required(VERSION 3.13.4)

project(4k_demo)

# Setup C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

add_compile_options(/utf-8 /W3 /WX)

add_compile_definitions(NOMINMAX)

# Disable exceptions
string(REPLACE "/EHsc" "/EHa-s-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Disable RTTI
string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	add_compile_definitions(DEBUG)
else()
		
	# Disable stack probes
	add_compile_options(/GS-)

	# Eleminate duplicated strings.
	add_compile_options(/GF)

	# Package global data in COMDAT sections for optimization.
	add_compile_options(/Gw)

	# Disable standard library and use "main" as entry point.
	add_link_options(/NODEFAULTLIB)
	add_link_options(/ENTRY:main)

	# Merge data and code segments together.
	add_link_options(/MERGE:.rdata=.text)
	add_link_options(/MERGE:.pdata=.text)
	add_link_options(/MERGE:.xdata=.text)

	# Dynamic base randomization isn't struictly necessary.
	add_link_options(/DYNAMICBASE:NO)

	# An undocumented flag.
	add_link_options(/EMITPOGOPHASEINFO)

	# Eleminate unused functions and data.
	add_link_options(/OPT:REF)
	add_link_options(/OPT:ICF)

	# use our own minimalistic Dos stub.
	# Use only DOS header binary, without actual DOS stub code (even trivial) to save several bytes.
	add_link_options(/STUB:${CMAKE_CURRENT_SOURCE_DIR}/DOSSTUB/DOSHEAD.BIN)

	# Manifest isn't necessary
	add_link_options(/MANIFEST:NO)

	# Reduce sections alignment
	add_link_options(/ALIGN:16)
	add_link_options(/FILEALIGN:16)

	add_link_options(/RELEASE)

	# Link-time optimization.
	# This significantly helps to reduce executable size, despite our executables are mostly single compilation unit.
	add_compile_options(/GL)
	add_link_options(/LTCG)

endif()

# Incremental linking isn't necessary.
add_link_options(/INCREMENTAL:NO)


add_executable(hello_world hello_world.cpp)
add_executable(simple_window WIN32 simple_window.cpp)
add_executable(mandelbrot WIN32 mandelbrot.cpp)
add_executable(tetris WIN32 tetris.cpp)
add_executable(terrain WIN32 terrain.cpp)
