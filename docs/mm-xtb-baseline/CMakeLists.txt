cmake_minimum_required(VERSION 3.24)

project(Youngcraft VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Test)

set(YOUNGCRAFT_CORE_SOURCES
    src/model/Molecule.cpp
    src/model/GeometryEditor.cpp
    src/io/GaussianParser.cpp
    src/io/XYZParser.cpp
    src/io/CubeParser.cpp
    src/io/FchkParser.cpp
    src/io/FileDispatcher.cpp
    src/io/StructureReader.cpp
    third_party/gemmi/symmetry.cpp
    src/analysis/AnalysisEngine.cpp
    src/settings/StyleManager.cpp
    src/animation/AnimationController.cpp
    src/export/ImageExporter.cpp
    src/render/IsosurfaceBuilder.cpp
)

add_library(youngcraft_core STATIC ${YOUNGCRAFT_CORE_SOURCES})
target_include_directories(youngcraft_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(youngcraft_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gemmi/include)
if(MSVC)
    set_source_files_properties(third_party/gemmi/symmetry.cpp PROPERTIES COMPILE_OPTIONS "/wd4244;/wd4267")
endif()
target_link_libraries(youngcraft_core PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)

if(MSVC)
    target_compile_options(youngcraft_core PRIVATE /utf-8 /W4)
else()
    target_compile_options(youngcraft_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

qt_add_executable(Youngcraft WIN32
    src/app/main.cpp
    src/app/MainWindow.cpp
    src/app/mainwindow/MainWindow_Ui.cpp
    src/app/mainwindow/MainWindow_Menus.cpp
    src/app/mainwindow/MainWindow_Toolbar.cpp
    src/app/mainwindow/MainWindow_File.cpp
    src/app/mainwindow/MainWindow_Navigation.cpp
    src/app/mainwindow/MainWindow_Animation.cpp
    src/app/mainwindow/MainWindow_Style.cpp
    src/app/mainwindow/MainWindow_Export.cpp
    src/app/mainwindow/MainWindow_Bonds.cpp
    src/app/mainwindow/MainWindow_Window.cpp
    src/app/mainwindow/MainWindow_Geometry.cpp
    src/app/mainwindow/MainWindow_MM.cpp
    src/app/ParseWorker.cpp
    src/render/MoleculeView.cpp
    src/render/moleculeview/MoleculeView_State.cpp
    src/render/moleculeview/MoleculeView_Camera.cpp
    src/render/moleculeview/MoleculeView_GL.cpp
    src/render/moleculeview/MoleculeView_Surface.cpp
    src/render/moleculeview/MoleculeView_Shadows.cpp
    src/render/moleculeview/MoleculeView_AtomsBonds.cpp
    src/render/moleculeview/MoleculeView_Scene.cpp
    src/render/moleculeview/MoleculeView_Overlays.cpp
    src/render/moleculeview/MoleculeView_Interaction.cpp
    src/ui/ScanResultsDialog.h
    src/ui/ScanResultsDialog.cpp
    src/ui/PlotWidget.cpp
    src/ui/AnalysisPanel.cpp
    src/ui/analysispanel/AnalysisPanel_Geometry.cpp
    src/ui/analysispanel/AnalysisPanel_Frequency.cpp
    src/ui/analysispanel/AnalysisPanel_Coordinates.cpp
    src/ui/analysispanel/AnalysisPanel_ScanIrc.cpp
    src/ui/analysispanel/AnalysisPanel_Tddft.cpp
    src/ui/analysispanel/AnalysisPanel_AtomsOrbitals.cpp
    src/ui/StyleEditorDialog.cpp
    src/ui/GeometryMeasureBar.cpp
    src/ui/styleeditor/StyleEditor_Lighting.cpp
    src/ui/styleeditor/StyleEditor_Atoms.cpp
    src/ui/styleeditor/StyleEditor_Bonds.cpp
    src/ui/styleeditor/StyleEditor_Isosurface.cpp
)
target_include_directories(Youngcraft PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(Youngcraft PRIVATE youngcraft_core Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)
set_target_properties(Youngcraft PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}"
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
if(MSVC)
    target_compile_options(Youngcraft PRIVATE /utf-8 /W4)
else()
    target_compile_options(Youngcraft PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    qt_add_executable(test_structure tests/structure/test_structure.cpp)
    target_link_libraries(test_structure PRIVATE youngcraft_core Qt6::Test)
    add_test(NAME structure_files COMMAND test_structure)
    set_tests_properties(structure_files PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    qt_add_executable(test_parser tests/parser/test_gaussian_parser.cpp)
    target_link_libraries(test_parser PRIVATE youngcraft_core Qt6::Test)
    target_include_directories(test_parser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME parser_gaussian COMMAND test_parser)
    set_tests_properties(parser_gaussian PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    qt_add_executable(test_model tests/model/test_model.cpp)
    target_link_libraries(test_model PRIVATE youngcraft_core Qt6::Test)
    target_include_directories(test_model PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME model_atoms_bonds COMMAND test_model)
    set_tests_properties(model_atoms_bonds PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    qt_add_executable(test_smoke tests/smoke/test_smoke.cpp
        src/app/MainWindow.cpp
    src/app/mainwindow/MainWindow_Ui.cpp
    src/app/mainwindow/MainWindow_Menus.cpp
    src/app/mainwindow/MainWindow_Toolbar.cpp
    src/app/mainwindow/MainWindow_File.cpp
    src/app/mainwindow/MainWindow_Navigation.cpp
    src/app/mainwindow/MainWindow_Animation.cpp
    src/app/mainwindow/MainWindow_Style.cpp
    src/app/mainwindow/MainWindow_Export.cpp
    src/app/mainwindow/MainWindow_Bonds.cpp
    src/app/mainwindow/MainWindow_Window.cpp
    src/app/mainwindow/MainWindow_Geometry.cpp
    src/app/mainwindow/MainWindow_MM.cpp
        src/app/ParseWorker.cpp
        src/render/MoleculeView.cpp
    src/render/moleculeview/MoleculeView_State.cpp
    src/render/moleculeview/MoleculeView_Camera.cpp
    src/render/moleculeview/MoleculeView_GL.cpp
    src/render/moleculeview/MoleculeView_Surface.cpp
    src/render/moleculeview/MoleculeView_Shadows.cpp
    src/render/moleculeview/MoleculeView_AtomsBonds.cpp
    src/render/moleculeview/MoleculeView_Scene.cpp
    src/render/moleculeview/MoleculeView_Overlays.cpp
    src/render/moleculeview/MoleculeView_Interaction.cpp
        src/ui/ScanResultsDialog.h
    src/ui/ScanResultsDialog.cpp
    src/ui/PlotWidget.cpp
        src/ui/AnalysisPanel.cpp
    src/ui/analysispanel/AnalysisPanel_Geometry.cpp
    src/ui/analysispanel/AnalysisPanel_Frequency.cpp
    src/ui/analysispanel/AnalysisPanel_Coordinates.cpp
    src/ui/analysispanel/AnalysisPanel_ScanIrc.cpp
    src/ui/analysispanel/AnalysisPanel_Tddft.cpp
    src/ui/analysispanel/AnalysisPanel_AtomsOrbitals.cpp
        src/ui/StyleEditorDialog.cpp
        src/ui/GeometryMeasureBar.cpp
    src/ui/styleeditor/StyleEditor_Lighting.cpp
    src/ui/styleeditor/StyleEditor_Atoms.cpp
    src/ui/styleeditor/StyleEditor_Bonds.cpp
    src/ui/styleeditor/StyleEditor_Isosurface.cpp
    )
    target_include_directories(test_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(test_smoke PRIVATE youngcraft_core Qt6::Test Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)
    add_test(NAME gui_smoke COMMAND test_smoke -platform offscreen)
    set_tests_properties(gui_smoke PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

if(WIN32)
    # Ensure the executable and tests use this build's Qt ABI even when an old
    # portable runtime exists or windeployqt cannot handle the SDK's Unicode path.
    set(_runtime_targets Youngcraft)
    if(BUILD_TESTING)
        list(APPEND _runtime_targets test_parser test_model test_smoke)
    endif()
    foreach(_target IN LISTS _runtime_targets)
        add_custom_command(TARGET ${_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${_target}> $<TARGET_FILE_DIR:${_target}>
            COMMAND_EXPAND_LISTS VERBATIM)
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
