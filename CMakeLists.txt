cmake_minimum_required(VERSION 3.24...4.4)
project(fluid_simulator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenMP REQUIRED)

find_program(GLSLC_EXECUTABLE
    NAMES glslc
    HINTS "$ENV{VULKAN_SDK}/bin"
    REQUIRED
)

set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SHADER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

set(SHADER_SOURCES
    particle.vert
    particle.frag
)

foreach(SHADER_SOURCE IN LISTS SHADER_SOURCES)
    set(SHADER_INPUT "${SHADER_SOURCE_DIR}/${SHADER_SOURCE}")
    set(SHADER_OUTPUT "${SHADER_BINARY_DIR}/${SHADER_SOURCE}.spv")

    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SHADER_BINARY_DIR}"
        COMMAND "${GLSLC_EXECUTABLE}" "${SHADER_INPUT}" -o "${SHADER_OUTPUT}"
        DEPENDS "${SHADER_INPUT}"
        COMMENT "Compiling ${SHADER_SOURCE}"
        VERBATIM
    )

    list(APPEND SHADER_BINARIES "${SHADER_OUTPUT}")
endforeach()

add_custom_target(fluid_shaders DEPENDS ${SHADER_BINARIES})

file(GLOB_RECURSE FLUID_SIMULATOR_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(fluid-simulator
    ${FLUID_SIMULATOR_SOURCES}
)

add_dependencies(fluid-simulator fluid_shaders)

target_include_directories(fluid-simulator
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(fluid-simulator
    PRIVATE
        Vulkan::Vulkan
        glfw
        glm::glm
        OpenMP::OpenMP_CXX
)

target_compile_definitions(fluid-simulator
    PRIVATE
        GLM_FORCE_RADIANS
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        FLUID_SHADER_DIR="${SHADER_BINARY_DIR}"
        $<$<CONFIG:Debug>:FLUID_ENABLE_VALIDATION>
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(fluid-simulator PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
    )
endif()
