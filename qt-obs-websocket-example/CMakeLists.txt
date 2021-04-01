cmake_minimum_required(VERSION 3.14)

project(qt-obs-websocket-example LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Test REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Test REQUIRED)

add_executable(${PROJECT_NAME}
  main.cpp
)

# Dependent include files
target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                         $<INSTALL_INTERFACE:include>)

target_link_libraries(${PROJECT_NAME} qt-obs-websocket
      Qt${QT_VERSION_MAJOR}::Test
      )

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
