cmake_minimum_required(VERSION 3.10)
project(synth LANGUAGES C CXX VERSION 1.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if(WIN32 OR MINGW)
	add_definitions(-DSDL_MAIN_HANDLED)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRC "src/*.h" "src/*.hpp" "src/*.cpp" "src/*.c")

if (MINGW)
	find_package(SDL2 REQUIRED SDL2)
else()
	find_package(SDL2 CONFIG REQUIRED)
endif()
find_package(SDL2_gfx REQUIRED STATIC)

set(BUILD_SHARED_LIBS CACHE BOOL OFF)
set(BUILD_TESTING CACHE BOOL OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/rtmidi)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sndfilter)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/osdialog)

add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME}
	PRIVATE
	SDL2::SDL2 SDL2::SDL2main
	${SDL2_GFX_LIBRARIES}
	rtmidi sndfilter osdialog
)
