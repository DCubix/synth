cmake_minimum_required(VERSION 3.10)
project(synth LANGUAGES C CXX VERSION 1.0)

if(WIN32 OR MINGW)
	add_definitions(-DSDL_MAIN_HANDLED)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
endif()

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
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/rtmidi)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sndfilter)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/osdialog)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/gui)

add_executable(${PROJECT_NAME} ${SRC})

if (MINGW)
	message("SDL2: " ${SDL2_LIBRARIES})
	target_link_libraries(${PROJECT_NAME}
		PRIVATE
		${SDL2_LIBRARIES}
		gui
		rtmidi sndfilter osdialog stdc++fs
	)
else()
	target_link_libraries(${PROJECT_NAME}
		PRIVATE
		SDL2::SDL2 SDL2::SDL2main
		gui
		rtmidi sndfilter osdialog
	)
endif()
