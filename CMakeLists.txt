cmake_minimum_required(VERSION 3.20)
project(jayrenderX12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_SYSTEM_VERSION 10.0.26100.0)

add_subdirectory(external/DirectX-Headers)

add_executable(${CMAKE_PROJECT_NAME} WIN32
	src/main.cpp
	src/application/application.h
	src/application/application.cpp
	src/renderer/renderer.h
	src/renderer/renderer.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC 
	${CMAKE_SOURCE_DIR}/src/helpers
	${CMAKE_SOURCE_DIR}/src/std_includes
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
	d3d12
	dxgi
	dxguid
	DirectX-Headers
	d3dcompiler
)

add_custom_command(
	TARGET ${CMAKE_PROJECT_NAME}
	PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	"${CMAKE_SOURCE_DIR}/src/shaders/quad_shaders.hlsl"
	"${CMAKE_BINARY_DIR}/"
	COMMENT "Copying shader file to binary directory..."
)

add_custom_command(
	TARGET ${CMAKE_PROJECT_NAME}
	PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	"${CMAKE_SOURCE_DIR}/src/shaders/cube_shaders.hlsl"
	"${CMAKE_BINARY_DIR}/"
	COMMENT "Copying shader file to binary directory..."
)