cmake_minimum_required(VERSION 3.22)
project(diplomka_pi LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(GLESV2 REQUIRED IMPORTED_TARGET glesv2)
find_package(Threads REQUIRED)
find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc imgcodecs video objdetect features2d calib3d)

include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.6
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

set(IMGUI_DIR ${imgui_SOURCE_DIR})
add_library(imgui_pi STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_pi PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_compile_definitions(imgui_pi PUBLIC IMGUI_IMPL_OPENGL_ES3)
target_link_libraries(imgui_pi PUBLIC PkgConfig::SDL2 PkgConfig::GLESV2)

add_executable(diplomka-pi
    main.cpp
    camera_pipe.cpp
    pi_vision_engine.cpp
    motor_engine.cpp
)
target_include_directories(diplomka-pi PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(diplomka-pi PRIVATE
    imgui_pi PkgConfig::SDL2 PkgConfig::GLESV2 Threads::Threads ${OpenCV_LIBS}
)
target_compile_options(diplomka-pi PRIVATE -Wall -Wextra -Wpedantic)

install(TARGETS diplomka-pi RUNTIME DESTINATION bin)
