add_library(youngcraft_app OBJECT
    MainWindow.cpp
    DocumentTabs.cpp
    mainwindow/MainWindow_Ui.cpp
    mainwindow/MainWindow_Chemcraft.cpp
    mainwindow/MainWindow_Menus.cpp
    mainwindow/MainWindow_Toolbar.cpp
    mainwindow/MainWindow_File.cpp
    mainwindow/MainWindow_Navigation.cpp
    mainwindow/MainWindow_Animation.cpp
    mainwindow/MainWindow_Style.cpp
    mainwindow/MainWindow_Export.cpp
    mainwindow/MainWindow_ProjectLibrary.cpp
    mainwindow/MainWindow_Bonds.cpp
    mainwindow/MainWindow_Window.cpp
    mainwindow/MainWindow_Geometry.cpp
    mainwindow/MainWindow_LabComplex.cpp
    mainwindow/MainWindow_FragmentBrowser.cpp
    mainwindow/MainWindow_Editor.cpp
    ParseWorker.cpp
    ${PROJECT_SOURCE_DIR}/resources/youngcraft.qrc
    ${PROJECT_SOURCE_DIR}/resources/fragments.qrc
)
youngcraft_enable_warnings(youngcraft_app)
target_link_libraries(youngcraft_app
    PUBLIC youngcraft_headers youngcraft_analysis youngcraft_animation youngcraft_compute youngcraft_export
           youngcraft_io youngcraft_ui youngcraft_viewport Qt6::Network
)

# These sources contain test-only worker hooks. Compile them in the final
# executable so the production app and smoke test keep separate definitions.
target_sources(Youngcraft PRIVATE
    mainwindow/MainWindow_MM.cpp
    mainwindow/MainWindow_Xtb.cpp
)
if(BUILD_TESTING)
    target_sources(test_smoke PRIVATE
        mainwindow/MainWindow_MM.cpp
        mainwindow/MainWindow_Xtb.cpp
    )
endif()
