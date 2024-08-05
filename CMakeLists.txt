cmake_minimum_required(VERSION 3.11.0)
project(ImPlot_Interface)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall")

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

include(FetchContent)

### Fetch ImGUI and create Library
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.0
)
FetchContent_MakeAvailable(imgui)

# imgui directories
file(GLOB IMGUI_SOURCES ${imgui_SOURCE_DIR}/*.cpp)
file(GLOB IMGUI_HEADERS ${imgui_SOURCE_DIR}/*.h)
file(GLOB IMGUI_BACKEND_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3_loader.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp)

# Create the library
add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_BACKEND_SOURCES})
target_include_directories(imgui PRIVATE ${imgui_SOURCE_DIR} )
set_property(TARGET imgui PROPERTY POSITION_INDEPENDENT_CODE ON)


### Fetch ImPlot and create Library
FetchContent_Declare(
        implot
        GIT_REPOSITORY https://github.com/epezent/implot.git
)
FetchContent_MakeAvailable(implot)

# implot directories
file(GLOB IMPLOT_SOURCES ${implot_SOURCE_DIR}/*.cpp)
file(GLOB IMPLOT_HEADERS ${implot_SOURCE_DIR}/*.h)

# Create the library
add_library(implot STATIC ${IMPLOT_SOURCES})
target_include_directories(implot PUBLIC ${implot_SOURCE_DIR} ${imgui_SOURCE_DIR})
target_link_directories(implot PUBLIC imgui)
set_property(TARGET implot PROPERTY POSITION_INDEPENDENT_CODE ON)

### Create ImPlot_Interface library
add_library(ImPlot_Interface STATIC src/implot_interface.cpp)
target_include_directories(ImPlot_Interface PUBLIC ${imgui_SOURCE_DIR} ${implot_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ImPlot_Interface PUBLIC imgui implot glfw ${OPENGL_LIBRARIES})
set_property(TARGET ImPlot_Interface PROPERTY POSITION_INDEPENDENT_CODE ON)

export(TARGETS ImPlot_Interface implot imgui
        NAMESPACE ImPlot_Interface::
        FILE ImPlot_InterfaceConfig.cmake
        )


# Add the executable
add_executable(example src/main.cpp)
target_include_directories(example PUBLIC ${imgui_SOURCE_DIR} ${implot_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(example ImPlot_Interface)






