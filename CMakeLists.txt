cmake_minimum_required(VERSION 3.20)
project(ETS2MobileCam LANGUAGES CXX C)

# --- x64 only ---
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "ETS2MobileCam is x64-only")
endif()

# --- MSVC only ---
if(NOT MSVC)
    message(FATAL_ERROR "ETS2MobileCam only supports the MSVC toolchain (Debug/Release)")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Debug + Release only (multi-config generators, e.g. Visual Studio) ---
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
endif()
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
endif()

# =============================================================================
# Submodule dependencies
# =============================================================================

# --- Asio (standalone, header-only) ---
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${CMAKE_SOURCE_DIR}/external/asio/include)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)

# =============================================================================
# Our own source files
# =============================================================================
set(ETS2MOBILECAM_SRC_DIR "${CMAKE_SOURCE_DIR}/src")

file(GLOB_RECURSE ETS2MOBILECAM_SOURCES CONFIGURE_DEPENDS
    "${ETS2MOBILECAM_SRC_DIR}/*.cpp"
)

add_library(ETS2MobileCam SHARED
    ${ETS2MOBILECAM_SOURCES}
)

set_target_properties(ETS2MobileCam PROPERTIES PREFIX "")

# =============================================================================
# Include directories
# =============================================================================
target_include_directories(ETS2MobileCam PRIVATE
    ${ETS2MOBILECAM_SRC_DIR}
    ${CMAKE_SOURCE_DIR}/Include
)

# =============================================================================
# Link libraries
# =============================================================================
target_link_libraries(ETS2MobileCam PRIVATE
    asio
    ws2_32
    d3d11
    dxgi
)

# =============================================================================
# Preprocessor definitions
# =============================================================================
target_compile_definitions(ETS2MobileCam PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    _WIN32_WINNT=0x0A00
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# =============================================================================
# MSVC compile/link options
# =============================================================================
target_compile_options(ETS2MobileCam PRIVATE
    /permissive-
    /Gr
    /W3
    /sdl
)

set_property(TARGET ETS2MobileCam PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)
