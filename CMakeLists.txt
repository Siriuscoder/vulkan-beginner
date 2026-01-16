cmake_minimum_required(VERSION 3.31.0)
project(vulkan_beginner)

option(ENABLE_SANITAIZE "Enable address sanitizer" OFF)

# Configuration
set(CMAKE_CONFIGURATION_TYPES Debug Release)
# Platform configuration
set(PLATFORM_TYPE PLATFORM_${CMAKE_SYSTEM_NAME} CACHE INTERNAL "PLATFORM_TYPE")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COMPILLER_COMMON_FLAGS "-fPIC -Wall -Wpedantic -Werror -pedantic-errors -Wno-format-security -Wno-unused-label")
if (ENABLE_SANITAIZE)
    set(COMPILLER_COMMON_FLAGS "${COMPILLER_COMMON_FLAGS} -fsanitize=address -fsanitize=undefined -fno-sanitize=alignment -fsanitize=leak")
endif()

set(DEBUG_GCC_COMPILER_FLAGS "${COMPILLER_COMMON_FLAGS} -DVALIDATION_LAYERS -DDEBUG -g3 -O0 -fstack-protector-all -fno-omit-frame-pointer -fno-optimize-sibling-calls")
set(RELEASE_GCC_COMPILER_FLAGS "${COMPILLER_COMMON_FLAGS} -DNDEBUG -g0 -O3")
set(CMAKE_C_FLAGS "-std=c99")
set(CMAKE_C_FLAGS_DEBUG ${DEBUG_GCC_COMPILER_FLAGS})
set(CMAKE_C_FLAGS_RELEASE ${RELEASE_GCC_COMPILER_FLAGS})
set(CMAKE_CXX_FLAGS_DEBUG ${DEBUG_GCC_COMPILER_FLAGS})
set(CMAKE_CXX_FLAGS_RELEASE ${RELEASE_GCC_COMPILER_FLAGS})
add_definitions(-D${PLATFORM_TYPE})

set(SHADERS_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADERS_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders/)
file(MAKE_DIRECTORY "${SHADERS_OUTPUT_DIR}")

find_package(Vulkan COMPONENTS glslc volk)
find_package(SDL2 REQUIRED)

if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan is not found")
endif()

if (NOT Vulkan_volk_FOUND)
    message(FATAL_ERROR "Volk is not found")
endif()

if (NOT Vulkan_glslc_FOUND)
    message(FATAL_ERROR "Vulkan GLSL compiler is not found")
endif()

if (NOT Vulkan_glslangValidator_FOUND)
    message(FATAL_ERROR "Vulkan GLSL validator is not found")
endif()

macro(add_shader shader_name)
    set(SHADERS_SOURCES ${SHADERS_DIR}/${shader_name}.vert ${SHADERS_DIR}/${shader_name}.frag)
    set(SHADERS_BINARIES ${CMAKE_BINARY_DIR}/shaders/${shader_name}.vert.spv ${CMAKE_BINARY_DIR}/shaders/${shader_name}.frag.spv)
    # Compile the shaders with glslc
    #
    # We use a custom command to compile the shaders with glslc. The
    # command takes the shader sources as input and generates the
    # compiled shaders as output. The command is executed in the
    # SHADERS_OUTPUT_DIR directory.
    #
    # We add a dependency between the vulkan_beginner target and the
    # shaders_compilation custom target. This means that the
    # vulkan_beginner target will be rebuilt whenever the shaders change.
    add_custom_command(
        OUTPUT ${SHADERS_BINARIES}
        COMMAND Vulkan::glslc
        ARGS -c ${SHADERS_SOURCES} -Werror
        WORKING_DIRECTORY ${SHADERS_OUTPUT_DIR}
        DEPENDS ${SHADERS_DIR} ${SHADERS_SOURCES}
        COMMENT "Compiling ${SHADERS_SOURCES} ..."
        VERBATIM
    )

    list(APPEND ALL_SHADERS_BINARIES ${SHADERS_BINARIES})
endmacro()

macro(add_sample sample_name)
    add_executable(${sample_name} ${sample_name}.c common.c shader_io.c /usr/include/volk.c)
    # Include directories for the Vulkan and Vulkan validation layers
    # libraries
    # We include the Vulkan and Vulkan validation layers include directories
    # as private include directories for the vulkan_beginner target.
    # This means that the Vulkan and Vulkan validation layers include
    # directories will be added to the include path for the vulkan_beginner
    # target, but will not be added to the include path for any other
    # targets that depend on the vulkan_beginner target.
    target_include_directories(${sample_name} PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
    )

    # Link the vulkan_beginner target against the Vulkan and Vulkan
    # validation layers libraries
    #
    # We link the vulkan_beginner target against the Vulkan and Vulkan
    # validation layers libraries as private libraries. This means that
    # the Vulkan and Vulkan validation layers libraries will be linked
    # against the vulkan_beginner target, but will not be propagated to any
    # other targets that depend on the vulkan_beginner target.
    target_link_libraries(${sample_name} PRIVATE  
        ${SDL2_LIBRARIES}
    )

    # Add a dependency between the executable target and the
    # shaders_compilation custom target
    add_dependencies(${sample_name} shaders_compilation)
endmacro()

add_shader(base)

add_custom_target(shaders_compilation
    COMMENT "Compiling shaders done"
    DEPENDS ${ALL_SHADERS_BINARIES}
)

add_sample(sample_minimal)
add_sample(sample_dyn_render)
