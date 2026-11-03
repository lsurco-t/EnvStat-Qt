cmake_minimum_required(VERSION 3.15)
project(EnvStat-Qt LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets SerialPort Test)

add_library(EnvStatCore STATIC
	src/app/MeasurementStore.cpp
	src/device/TelemetryParser.cpp
)

target_include_directories(EnvStatCore PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(EnvStatCore PUBLIC
	Qt6::Core
)

add_executable(EnvStat-Qt
	src/main.cpp
	src/device/SerialWorker.cpp
	src/ui/MainWindow.cpp
)

target_link_libraries(EnvStat-Qt PRIVATE
	EnvStatCore
	Qt6::Core
	Qt6::Widgets
	Qt6::SerialPort
)

target_include_directories(EnvStat-Qt PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/src/device
	${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

add_executable(Test_Telemetry
	tests/tst_TelemetryParser.cpp
)

target_link_libraries(Test_Telemetry PRIVATE
	EnvStatCore
	Qt6::Core
	Qt6::Test
)

add_test(
	NAME TelemetryParser
	COMMAND Test_Telemetry
)

add_executable(Test_Measurement
	tests/tst_MeasurementStore.cpp
)

target_link_libraries(Test_Measurement PRIVATE
	EnvStatCore
	Qt6::Core
	Qt6::Test
)

add_test(
	NAME MeasurementStore
	COMMAND Test_Measurement
)