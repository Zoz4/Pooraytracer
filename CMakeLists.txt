# CMakeList.txt: Pooraytracer 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.18)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Pooraytracer")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 第三方库目录
add_subdirectory(ThirdPartyLibraries)


# 将源代码添加到此项目的可执行文件。
file(GLOB SOURCES "Source/*.cpp" "Source/*.h")
add_executable (${PROJECT_NAME} "main.cpp" ${SOURCES})

target_link_libraries(Pooraytracer PRIVATE 
	glm::glm
	tinyobjloader
	tinyxml2::tinyxml2
	stb
	spdlog::spdlog
)

# 设置文件目录
target_compile_definitions(${PROJECT_NAME} PRIVATE RESOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/example-scenes-cg24/")
target_compile_definitions(${PROJECT_NAME} PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}/")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Pooraytracer PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
