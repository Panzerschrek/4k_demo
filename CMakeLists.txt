cmake_minimum_required(VERSION 3.13.4)

project(4k_demo)

# Setup C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

if(MSVC)

	# Disable exceptions
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

	# Disable RTTI
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

	# Disable stack probes
	add_compile_options(/GS-)

	add_compile_options(/utf-8 /W3 /WX)

	add_compile_options(/FAs) # Generate assembly to analyze

	# Disable standard library and use "main" as entry point.
	add_link_options(/NODEFAULTLIB)
	add_link_options(/ENTRY:main)

	# Merge data and code segments together.
	add_link_options(/MERGE:.rdata=.text)
	add_link_options(/MERGE:.pdata=.text)
	add_link_options(/MERGE:.xdata=.text)

	# use our own minimalistic Dos stub.
	add_link_options(/STUB:${CMAKE_CURRENT_SOURCE_DIR}/DOSSTUB/DOSSTUB2.EXE)

	# Manifest isn't necessary
	add_link_options(/MANIFEST:NO)

	# Reduce sections alignment
	add_link_options(/ALIGN:16)
	add_link_options(/FILEALIGN:16)

else()
	add_compile_options(-Werror -Wall -Wextra -Wconversion)
	add_compile_options(-fvisibility=hidden -fvisibility-inlines-hidden)
	add_compile_options(-fdata-sections -ffunction-sections) # simplify later gc-sections.
	add_compile_options(-fno-rtti)

	# Clear garbage.
	string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " -s -Wl,--gc-sections")
	string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE " -s -Wl,--gc-sections")
endif()

add_executable(4k_demo main.cpp)
