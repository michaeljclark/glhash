#            ███▌    ▐█                        █▌
#            ▀▀█▌    ▐█                        █▌
#              █▌    ▐█                        █▌
#    ▗██▙▐█    █▌    ▐█▗▟█▙▖  ▗▟██▙▖   ▗▟██▙▖  █▌▄██▄
#   ▗█▛▀▜██    █▌    ▐█▟▀▀█▙  █▛▀▀▜█▖  █▛▀▀▀▌  █▙▛▀▜█▖
#   ▟█   ▜█    █▌    ▐█▘  ▐█  ▘    ▜▌ ▐█       █▛   █▌
#   █▌   ▐█    █▌    ▐█   ▝█   ▗▄▄▄▟▙  █▙▄▖    █▌   ▜▌
#   █▌   ▐█    █▌    ▐█    █  ▟██████   ▀▜██▖  █▌   ▐▌
#   █▌   ▐█    █▌    ▐█    █ ▐█▀   ▐█      ▜█  █▌   ▐▌
#   ▜█   ▟█    █▌    ▐█    █ ▐█    ██ ▗    ▐█  █▌   ▐▌
#   ▝█▙▄▟██    ▐█▄▄▖ ▐█    █ ▝█▙▄▄█▜█ ▐█▄▄▄█▛  █▌   ▐▌
#    ▝██▛▐█     ▜██▌ ▐█    █  ▝██▛▘▐█  ▀███▀   █▌   ▐▌
#        ▐█
#    ▄  ▗▟▛
#    █████▘
#     ▀▀▀

cmake_minimum_required (VERSION 3.12)
project(glhash VERSION 0.0.1)

include(CheckLibraryExists)
check_library_exists(m sqrtf "" HAVE_LIB_M)
if (HAVE_LIB_M)
    set(EXTRA_LIBS ${EXTRA_LIBS} m)
endif (HAVE_LIB_M)

# find glslangValidator
set(VULKAN_SDK "/opt/vulkan/1.2.154.0/x86_64" CACHE PATH "Vulkan SDK Location")
find_program (GLSL_LANG_VALIDATOR NAMES glslangValidator PATHS ${VULKAN_SDK}/bin)

# Find GLFW library
find_package(PkgConfig)
pkg_check_modules(GLFW3 glfw3)

# Find OpenGL library
include(FindOpenGL)

# add pthreads on linux systems
find_package(Threads REQUIRED)

# set defaults for options
set (OPENGL_EXAMPLES_DEFAULT ON)
set (SPIRV_BINARY_DEFAULT ON)
if (NOT OpenGL_OpenGL_FOUND)
    set (OPENGL_EXAMPLES_DEFAULT OFF)
endif ()
if (NOT GLSL_LANG_VALIDATOR)
    set (SPIRV_BINARY_DEFAULT OFF)
endif ()

# user configurable options
option(OPENGL_EXAMPLES "Build OpenGL examples" ${OPENGL_EXAMPLES_DEFAULT})
option(SPIRV_BINARIES "Use SPIR-V binaries" ${SPIRV_BINARY_DEFAULT})
option(EXTERNAL_GLFW "Use external GLFW project" ON)
option(EXTERNAL_GLAD "Use external GLAD project" ON)

message(STATUS "GLSL_LANG_VALIDATOR = ${GLSL_LANG_VALIDATOR}")
message(STATUS "VULKAN_SDK = ${VULKAN_SDK}")
message(STATUS "OPENGL_EXAMPLES = ${OPENGL_EXAMPLES}")
message(STATUS "EXTERNAL_GLFW = ${EXTERNAL_GLFW}")
message(STATUS "EXTERNAL_GLAD = ${EXTERNAL_GLAD}")
message(STATUS "SPIRV_BINARIES = ${SPIRV_BINARIES}")

# Support for external glfw
if (EXTERNAL_GLFW)
    include(ExternalProject)
    ExternalProject_Add(
        GLFW
        GIT_REPOSITORY "https://github.com/glfw/glfw.git"
        SOURCE_DIR "${CMAKE_BINARY_DIR}/third_party/glfw3"
        STEP_TARGETS build
        EXCLUDE_FROM_ALL TRUE
        CMAKE_ARGS -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF
                   -DGLFW_BUILD_DOCS=OFF -DGLFW_INSTALL=OFF
    )
    set(GLFW_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/third_party/glfw3/include")
    set(GLFW_LIBRARY_DIR "${CMAKE_BINARY_DIR}/GLFW-prefix/src/GLFW-build/src/")
    set(GLFW_LIBRARIES glfw3)
    include_directories(${GLFW_INCLUDE_DIRS})
    link_directories(${GLFW_LIBRARY_DIR})
    list(APPEND GLFW_LIBS_ALL ${GLFW_LIBRARIES} ${EXTRA_LIBS})
else ()
    add_definitions(${GLFW3_CFLAGS})
    list(APPEND GLFW_LIBS_ALL ${GLFW3_LDFLAGS} ${EXTRA_LIBS})
endif ()

# Support for external glad
if (EXTERNAL_GLAD)
    include(ExternalProject)
    ExternalProject_Add(
        GLAD
        GIT_REPOSITORY "https://github.com/Dav1dde/glad.git"
        SOURCE_DIR "${CMAKE_BINARY_DIR}/third_party/glad"
        STEP_TARGETS build
        EXCLUDE_FROM_ALL TRUE
        CMAKE_ARGS -DGLAD_EXPORT=OFF -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    )
    set(GLAD_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/GLAD-prefix/src/GLAD-build/include/")
    set(GLAD_LIBRARY_DIR "${CMAKE_BINARY_DIR}/GLAD-prefix/src/GLAD-build/")
    set(GLAD_LIBRARIES glad)
    include_directories(${GLAD_INCLUDE_DIRS})
    link_directories(${GLAD_LIBRARY_DIR})
    list(APPEND OPENGL_LOADER_LIBS ${GLAD_LIBRARIES})
else ()
    list(APPEND OPENGL_LOADER_LIBS ${OPENGL_opengl_LIBRARY})
endif ()

if (SPIRV_BINARIES)
    file(GLOB_RECURSE COMPUTE_SHADER_SOURCE "src/*.comp")
    set(SHADER_SOURCE ${COMPUTE_SHADER_SOURCE})
    source_group("shaders" FILES ${SHADER_SOURCE})
    foreach(shader ${SHADER_SOURCE})
        file(RELATIVE_PATH shader_name ${CMAKE_SOURCE_DIR}/src ${shader})
        file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/${shader_name}.spv" shader_bin)
        add_custom_command(OUTPUT ${shader_bin} DEPENDS ${shader}
            COMMAND ${GLSL_LANG_VALIDATOR} -V ${shader} -o ${shader_bin})
        list(APPEND SHADER_BINARIES ${shader_bin})
    endforeach(shader)
    add_custom_target(generated_shaders DEPENDS ${SHADER_BINARIES})
endif (SPIRV_BINARIES)

file(RELATIVE_PATH BIN_PATH ${CMAKE_SOURCE_DIR} ${PROJECT_BINARY_DIR})
add_definitions(-DSRC_PATH=\"src\" -DBIN_PATH=\"${BIN_PATH}\")

if (OPENGL_EXAMPLES)
    foreach(prog IN ITEMS gl4_hash)
        message("-- Adding: ${prog}")
        add_executable(${prog} src/${prog}.c)
        target_link_libraries(${prog} ${GLFW_LIBS_ALL} ${OPENGL_LOADER_LIBS})
        if (EXTERNAL_GLAD)
            target_compile_definitions(${prog} PRIVATE -DHAVE_GLAD)
        endif ()
        if (EXTERNAL_GLFW)
            add_dependencies(${prog} GLFW-build)
        endif ()
        if (EXTERNAL_GLAD)
            add_dependencies(${prog} GLAD-build)
        endif ()
        if (SPIRV_BINARIES)
            add_dependencies(${prog} generated_shaders)
        endif (SPIRV_BINARIES)
    endforeach(prog)
endif (OPENGL_EXAMPLES)
