cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_COMPILER "clang++")

project(singsingplayer LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Building in the source directory is not supported, try building in a 'build' directory.")
endif()

set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/prefix" CACHE PATH "Path to prefix")
set(CMAKE_FIND_ROOT_PATH "${CMAKE_PREFIX_PATH}")

file(GLOB_RECURSE sources
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)
file(GLOB_RECURSE headers
    ${CMAKE_SOURCE_DIR}/src/*.hpp
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(OpenGL_GL_PREFERENCE "GLVND")

add_executable(${PROJECT_NAME} ${sources} ${headers})

include_directories(
    "${CMAKE_PREFIX_PATH}/include"
    "/usr/local/include"
)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)
# find_package(SDL2_image REQUIRED)
# find_package(SDL2_ttf REQUIRED)
# find_package(FreetypeGL REQUIRED)
find_package(SoundIO REQUIRED)
find_package(Aubio REQUIRED)
find_package(FFmpeg REQUIRED COMPONENTS AVCODEC AVFORMAT AVUTIL AVFILTER AVDEVICE SWRESAMPLE SWSCALE)
find_package(MPV REQUIRED)
# find_package(ProjectM)


target_link_libraries(
    ${PROJECT_NAME}
    -static-libgcc -static-libstdc++
    ${OPENGL_LIBRARY}
    ${GLEW_LIBRARY}
    ${SDL2_LIBRARY}
    # ${SDL2_IMAGE_LIBRARIES}
    # ${SDL2_TTF_LIBRARIES}
    # ${FREETYPEGL_LIBRARY}
    ${SOUNDIO_LIBRARY}
    ${AUBIO_LIBRARY}
    ${FFMPEG_LIBRARY}
    ${MPV_LIBRARY}
    # ${PROJECTM_LIBRARY}
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
