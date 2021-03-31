cmake_minimum_required(VERSION 3.14)

project(qt-obs-websocket LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Network WebSockets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network WebSockets REQUIRED)

add_library(${PROJECT_NAME} STATIC
  qtobswebsocket.cpp
  qtobswebsocket.h
  obsresponse.h
)

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                         $<INSTALL_INTERFACE:include>)

target_link_libraries(${PROJECT_NAME} 
  PUBLIC 
    Qt${QT_VERSION_MAJOR}::Core 
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::WebSockets
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# This makes the project importable from the build directory
export(TARGETS ${PROJECT_NAME} FILE "${PROJECT_NAME}_export.cmake")
