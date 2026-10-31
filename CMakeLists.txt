cmake_minimum_required(VERSION 3.15)
project(EnvStat-Qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets SerialPort)

set(SOURCES
	src/main.cpp
	src/app/MeasurementStore.cpp
	src/device/SerialWorker.cpp
	src/device/TelemetryParser.cpp
	src/ui/MainWindow.cpp
)

set(HEADERS
	src/app/Measurement.h
	src/device/SerialWorker.h
	src/device/TelemetryParser.h
	src/ui/MainWindow.h
)

add_executable(EnvStat-Qt
	${SOURCES}
	${HEADERS}
)

target_include_directories(EnvStat-Qt PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(EnvStat-Qt PRIVATE
	Qt6::Core
	Qt6::Widgets
	Qt6::SerialPort
)
