cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_COMPILER "g++")

if(UNIX AND NOT APPLE)
    set(STATIC_FLAGS "-static-libgcc -static-libstdc++")
endif()

project(singsing LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Building in the source directory is not supported, try building in a 'build' directory.")
endif()

set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/prefix" CACHE PATH "Path to prefix")
set(CMAKE_FIND_ROOT_PATH "${CMAKE_PREFIX_PATH}")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_SKIP_INSTALL_RPATH TRUE)
set(OpenGL_GL_PREFERENCE "GLVND")

file(GLOB sources
    ${CMAKE_SOURCE_DIR}/src/base/*.cpp
    ${CMAKE_SOURCE_DIR}/src/util/*.cpp
    ${CMAKE_SOURCE_DIR}/src/lib/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_executable(${PROJECT_NAME} ${sources})

include_directories(
    "${CMAKE_PREFIX_PATH}/include"
    "/usr/local/include"
)

add_subdirectory(lib/glad/)

find_package(OpenGL REQUIRED)
# find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)
# find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(Freetype REQUIRED)
# find_package(FreetypeGL REQUIRED)
find_package(SoundIO REQUIRED)
find_package(Aubio REQUIRED)
# find_package(FFmpeg REQUIRED COMPONENTS AVCODEC AVFORMAT AVUTIL AVFILTER AVDEVICE SWRESAMPLE SWSCALE)
find_package(MPV REQUIRED)

target_link_libraries(
    ${PROJECT_NAME}
    ${STATIC_FLAGS}
    glad
    ${OPENGL_LIBRARY}
    # ${GLEW_LIBRARY}
    ${SDL2_LIBRARY}
    # ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${FREETYPE_LIBRARY}
    # ${FREETYPEGL_LIBRARY}
    ${SOUNDIO_LIBRARY}
    ${AUBIO_LIBRARY}
    #${FFMPEG_LIBRARY}
    ${MPV_LIBRARY}
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

if(APPLE)
    # Build for macOS (unfinished)
    set(CPACK_GENERATOR "Bundle")
    set(CPACK_BUNDLE_NAME "singsing")
    set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/platform/macos/Info.plist")
    set(CPACK_BUNDLE_ICON "/dev/null")

    include(CPACK)
endif(APPLE)
