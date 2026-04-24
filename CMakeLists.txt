cmake_minimum_required(VERSION 3.20)
project(jayrenderX12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_SYSTEM_VERSION 10.0.26100.0)

add_subdirectory(external/DirectX-Headers)

add_executable(${CMAKE_PROJECT_NAME} WIN32
	# src/command_queue/command_queue.cpp
	# src/application/application.cpp
	src/main.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/helpers)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
	d3d12
	dxgi
	dxguid
	DirectX-Headers
)
