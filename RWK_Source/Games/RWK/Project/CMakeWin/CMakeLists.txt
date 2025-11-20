set(appname "RWK")
cmake_minimum_required(VERSION 3.20.0)
project(${appname} VERSION 0.1.0 LANGUAGES C CXX)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../../Resources/)

# Getting all c++ source files

file(GLOB_RECURSE SRCS ${PROJECT_SOURCE_DIR}/../../Source/*.cpp)
file(GLOB _OS ${PROJECT_SOURCE_DIR}/../../../../Framework/OS/Windows/*.cpp)
file(GLOB _RAPT ${PROJECT_SOURCE_DIR}/../../../../Framework/RAPT/*.cpp)
file(GLOB_RECURSE MAIN ${PROJECT_SOURCE_DIR}/Main_Win.cpp)

# Including framework CMakeLists.txt or sources?
# no seriously dad, what is this?
include_directories(${PROJECT_SOURCE_DIR}/../../../../Framework/OS/Windows/Include)
include_directories(${PROJECT_SOURCE_DIR}/../../../../Framework/RAPT/)
include_directories(${PROJECT_SOURCE_DIR}/../../../../Framework/OS/Windows)
include_directories(${PROJECT_SOURCE_DIR}/../../../../Framework/OS/)
include_directories(${PROJECT_SOURCE_DIR}/../../Source/)

# Main executable
add_executable(${appname} WIN32 ${SRCS} ${MAIN} ${_OS} ${_RAPT})

# Show all warnings, works for clang and GCC
#target_compile_options(${appname} PRIVATE -Wall -Wno-extra -Wno-comment -Wno-write-strings -Wno-class-memaccess -Wno-missing-field-initializers -Wno-unused-variable -Wno-deprecated-copy -Wno-unused-parameter -Wno-ignored-qualifiers -Wno-tautological-compare -Wno-conversion-null -Wno-misleading-indentation -fmax-errors=1 -Wfatal-errors -Wno-delete-incomplete -Wno-int-to-pointer-cast -DGL30 -Wno-format-security -fpermissive) #gcc

#clang
#target_compile_options(${appname} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-pthread -Wall -Wno-extra -Wno-comment -Wno-write-strings -Wno-missing-field-initializers -Wno-unused-variable -Wno-deprecated-copy -Wno-unused-parameter -Wno-ignored-qualifiers -Wno-tautological-compare -Wno-conversion-null -Wno-misleading-indentation -Wfatal-errors -Wno-delete-incomplete -Wno-int-to-pointer-cast -Wno-format-security -fpermissive -Wno-bitwise-instead-of-logical -Wno-overloaded-virtual -Wno-dynamic-class-memaccess -Wno-delete-non-abstract-non-virtual-dtor -Wno-char-subscripts -Wno-constant-conversion -Wno-unused-but-set-variable -Wno-unsequenced -Wno-unknown-warning-option>)
#target_compile_options(${appname} PRIVATE $<$<COMPILE_LANGUAGE:C>:-pthread -Wall -Wno-unused-variable>) #clang

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D_DEBUG)
endif ()
#add_definitions(-DLEGACY_GL -DGL_LEGACY -DRAPT_LEGACY -DNO_RAPT_ML) # for legacy system builds only
add_definitions(-DGL30) # for non-legacy system builds only

target_compile_definitions(${appname} PRIVATE _WIN32 AppRunsInThread)

# Set c++ standard for executable
set_target_properties(${appname} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS ON INSTALL_RPATH "$ORIGIN" BUILD_WITH_INSTALL_RPATH TRUE)

# Link to the actual SDL2 library.
# SDL2::SDL2 is the shared SDL library, SDL2::SDL2-static is the static SDL library.

#include BASS
find_library(BASS_LIBRARY NAMES bass.lib PATHS ${PROJECT_SOURCE_DIR}/../../../../Framework/OS/Windows/BASS/ ${CURRENT_BINARY_DIR})
find_library(GFX_LIBRARY NAMES glew32.lib OpenGL32.lib SDL2.lib SLD2_image.lib PATHS ${PROJECT_SOURCE_DIR}/../../../../Framework/OS/Windows/SDL_2020/Lib/ ${CURRENT_BINARY_DIR})

#
# This strips the /framework/os/blah info from the final executable...
#
set(CMAKE_BUILD_RPATH "${PROJECT_SOURCE_DIR}/../../../../Framework/OS/Linux/ThirdParty/BASS/libs/x86_64/;${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
#
#

# Link to OpenGL
#target_link_libraries(${appname} PRIVATE OpenGL::OpenGL ${BASS_LIBRARY} ${CURL_LIBRARIES} ${PROJECT_SOURCE_DIR}/../../../../Framework/OS/Linux/ThirdParty/SDLImage/libSDL2_image.a )
target_link_libraries(${appname} PRIVATE
        ${PROJECT_SOURCE_DIR}/../../../../Framework/OS/Windows/SDL_2020/Lib/OpenGL32.lib
        ${PROJECT_SOURCE_DIR}/../../../../Framework/OS/Windows/SDL_2020/Lib/glew32.lib
        ${PROJECT_SOURCE_DIR}/../../../../Framework/OS/Windows/SDL_2020/Lib/sdl2.lib
        ${PROJECT_SOURCE_DIR}/../../../../Framework/OS/Windows/SDL_2020/Lib/sdl2_image.lib
        ${PROJECT_SOURCE_DIR}/../../../../Framework/OS/Windows/BASS/bass.lib)

