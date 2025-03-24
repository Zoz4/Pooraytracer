# glm
add_subdirectory(glm)
set( "GLM_DEFINITIONS"
    GLM_FORCE_MESSAGES
    GLM_FORCE_CXX20
    GLM_ENABLE_EXPERIMENTAL
    GLM_FORCE_SWIZZLE
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)
target_compile_definitions(glm PUBLIC ${GLM_DEFINITIONS})
target_compile_options(glm 
	PRIVATE 
	$<$<CXX_COMPILER_ID:MSVC>:/bigobj> 
	$<$<CXX_COMPILER_ID:GNU>:-Wa,-mbig-obj>
) 

# tinyobjloader
add_subdirectory(tinyobjloader)
target_compile_definitions(tinyobjloader PUBLIC TINYOBJLOADER_USE_DOUBLE) 

# tinyxml2
set(tinyxml2_BUILD_TESTING OFF CACHE BOOL "Build tests for tinyxml2" FORCE)
add_subdirectory(tinyxml2)

# stb 
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")

# spdlog
add_subdirectory(spdlog)
## 设置 SPDLOG_ACTIVE_LEVEL 根据构建配置自动调整
target_compile_definitions(spdlog PUBLIC 
    $<$<CONFIG:DEBUG>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG>
    $<$<CONFIG:RELEASE>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>
)