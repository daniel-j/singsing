cmake_minimum_required(VERSION 2.8)
set (CMAKE_CXX_STANDARD 11)

project(singsingplayer)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Building in the source directory is not supported, try building in a 'build' directory.")
endif()

set(CMAKE_SKIP_RPATH TRUE)

file(GLOB_RECURSE sources
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)
file(GLOB_RECURSE headers
    ${CMAKE_SOURCE_DIR}/src/*.hpp
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
# find_package(PortAudio REQUIRED)
find_package(aubio REQUIRED)
find_package(SoundIo REQUIRED)
find_package(MPV REQUIRED)

add_executable(${PROJECT_NAME} ${sources} ${headers})

target_link_libraries(
    ${PROJECT_NAME}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${GLM_LIBRARIES}
    ${SDL2_LIBRARY}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${AUBIO_LIBRARIES}
    # ${PORTAUDIO_LIBRARIES}
    ${SOUNDIO_LIBRARY}
    ${MPV_LIBRARY}
)
