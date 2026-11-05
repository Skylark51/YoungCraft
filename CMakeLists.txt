cmake_minimum_required(VERSION 3.24)

project(Youngcraft VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Match the strict MSVC parsing mode used by the local Ninja release build.
# CI must compile under the same rules to catch ambiguous C++ declarations.
if(MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/permissive->
                        $<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>)
endif()
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Network)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Test)
endif()

add_library(youngcraft_headers INTERFACE)
target_include_directories(youngcraft_headers INTERFACE "${PROJECT_SOURCE_DIR}/src")

function(youngcraft_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8 /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_subdirectory(src/model)
add_subdirectory(src/io)
add_subdirectory(src/compute)
add_subdirectory(src/analysis)
add_subdirectory(src/animation)
add_subdirectory(src/settings)
add_subdirectory(src/export)
add_subdirectory(src/render)
add_subdirectory(src/ui)

qt_add_executable(Youngcraft WIN32 src/app/main.cpp)
if(BUILD_TESTING)
    include(tests/Targets.cmake)
endif()
add_subdirectory(src/app)

if(WIN32)
    target_sources(Youngcraft PRIVATE resources/youngcraft.rc)
    target_include_directories(Youngcraft PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/resources)
endif()
target_link_libraries(Youngcraft PRIVATE youngcraft_app)
set(YOUNGCRAFT_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}" CACHE PATH "Youngcraft executable output directory")
get_filename_component(_youngcraft_runtime_output "${YOUNGCRAFT_RUNTIME_OUTPUT_DIRECTORY}" ABSOLUTE
    BASE_DIR "${CMAKE_SOURCE_DIR}")
set_target_properties(Youngcraft PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${_youngcraft_runtime_output}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${_youngcraft_runtime_output}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${_youngcraft_runtime_output}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${_youngcraft_runtime_output}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${_youngcraft_runtime_output}"
)
if(WIN32 AND TARGET Qt6::windeployqt)
    add_custom_command(TARGET Youngcraft POST_BUILD
        COMMAND $<TARGET_FILE:Qt6::windeployqt>
            --compiler-runtime
            --no-translations
            --dir "$<TARGET_FILE_DIR:Youngcraft>"
            "$<TARGET_FILE:Youngcraft>"
        COMMENT "Deploying Qt runtime beside Youngcraft.exe"
        VERBATIM
    )
endif()
youngcraft_enable_warnings(Youngcraft)

option(YOUNGCRAFT_BUILD_RENDER_PREVIEW "Build the isolated molecular rendering capture tool" OFF)
if(YOUNGCRAFT_BUILD_RENDER_PREVIEW)
    qt_add_executable(render_quality_preview tools/render_quality_preview.cpp)
    target_link_libraries(render_quality_preview PRIVATE youngcraft_viewport)
    target_compile_options(render_quality_preview PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
endif()

if(WIN32)
    # Ensure the executable and tests use this build's Qt ABI even when an old
    # portable runtime exists or windeployqt cannot handle the SDK's Unicode path.
    set(_runtime_targets Youngcraft)
    if(TARGET render_quality_preview)
        list(APPEND _runtime_targets render_quality_preview)
    endif()
    if(BUILD_TESTING)
        list(APPEND _runtime_targets test_orbitals test_parser test_orca_parser test_model test_smoke test_instance test_structure)
    endif()
    foreach(_target IN LISTS _runtime_targets)
        add_custom_command(TARGET ${_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${_target}> $<TARGET_FILE_DIR:${_target}>
            COMMAND_EXPAND_LISTS VERBATIM)
        if(TARGET Qt6::QGifPlugin)
            add_custom_command(TARGET ${_target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${_target}>/imageformats"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:Qt6::QGifPlugin>" "$<TARGET_FILE_DIR:${_target}>/imageformats"
                VERBATIM)
        endif()
        foreach(_plugin QWindowsIntegrationPlugin QOffscreenIntegrationPlugin)
            if(TARGET Qt6::${_plugin})
                add_custom_command(TARGET ${_target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${_target}>/platforms"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE:Qt6::${_plugin}>" "$<TARGET_FILE_DIR:${_target}>/platforms"
                    VERBATIM)
            endif()
        endforeach()
    endforeach()
endif()

foreach(_mm_target Youngcraft)
    add_custom_command(TARGET ${_mm_target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${_mm_target}>/tools"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/tools/mm_optimize.py"
            "$<TARGET_FILE_DIR:${_mm_target}>/tools/mm_optimize.py"
        VERBATIM)
endforeach()
if(BUILD_TESTING)
    add_custom_command(TARGET test_smoke POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:test_smoke>/tools"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/tools/mm_optimize.py"
            "$<TARGET_FILE_DIR:test_smoke>/tools/mm_optimize.py"
        VERBATIM)
endif()

install(TARGETS Youngcraft RUNTIME DESTINATION .)
install(DIRECTORY resources/fragments DESTINATION resources)
install(FILES licenses/Avogadro-fragments-BSD-3-Clause.txt DESTINATION licenses)
install(DIRECTORY xtb_runtime/ DESTINATION xtb_runtime)

add_custom_command(TARGET Youngcraft POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/gaussian-remote.json"
        "$<TARGET_FILE_DIR:Youngcraft>/youngcraft-server.json")
