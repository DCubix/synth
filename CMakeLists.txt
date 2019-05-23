cmake_minimum_required(VERSION 3.10)
project(synth LANGUAGES C CXX VERSION 1.0)

option(PERF_TESTS "Enable performance testing" OFF)

if (PERF_TESTS)
	add_definitions(-DSYN_PERF_TESTS)
endif()

if(WIN32 OR MINGW)
	add_definitions(-DSDL_MAIN_HANDLED)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(VCPKG_TARGET_TRIPLET x64-windows)

file(GLOB SRC
	"src/engine/*.h"
	"src/engine/*.hpp"
	"src/engine/*.cpp"
	"src/engine/*.c"

	"src/log/*.h"
	"src/log/*.hpp"
	"src/log/*.cpp"
	"src/log/*.c"

	"src/*.h"
	"src/*.hpp"
	"src/*.cpp"
	"src/*.c"
)

find_package(SDL2 CONFIG REQUIRED)

set(BUILD_SHARED_LIBS CACHE BOOL OFF)
set(BUILD_TESTING CACHE BOOL OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sndfilter)

if(NOT PERF_TESTS)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/rtmidi)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/osdialog)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/gui)
endif()

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE sndfilter)

if (MINGW)
	target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})
	if (NOT PERF_TESTS)
		target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
	endif()
else()
	target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main stdc++fs)
endif()

if (NOT PERF_TESTS)
	target_link_libraries(${PROJECT_NAME} PRIVATE gui osdialog rtmidi)
endif()

if (CMAKE_DL_LIBS)
	target_link_libraries(${PROJECT_NAME} PRIVATE 
		${CMAKE_DL_LIBS}
	)
endif()

get_target_property(OUT ${PROJECT_NAME} LINK_LIBRARIES)
message(STATUS ${OUT})
