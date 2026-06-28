cmake_minimum_required(VERSION 3.20)
project(PathPlanner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

set(CORE_SOURCES
    src/core/BezierSegment.cpp
    src/core/Path.cpp
    src/core/PathInterpolator.cpp
    src/core/Project.cpp
)

set(CORE_HEADERS
    src/core/Vec2.h
    src/core/Waypoint.h
    src/core/BezierSegment.h
    src/core/Path.h
    src/core/PathInterpolator.h
    src/core/RobotConfig.h
    src/core/Project.h
)

qt_add_executable(PathPlanner
    src/main.cpp
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)

target_include_directories(PathPlanner PRIVATE src)

target_link_libraries(PathPlanner PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)

if(WIN32)
    set_target_properties(PathPlanner PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
