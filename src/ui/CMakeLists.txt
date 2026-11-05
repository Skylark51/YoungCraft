add_library(youngcraft_ui STATIC
    ReactionCaptureDialog.cpp
    MultiCaptureDialog.cpp
    MechanismAnimationDialog.cpp
    ReactionCoordinateWidget.cpp
    ReactionCoordinateWidget.h
    MultiCaptureDialog.h
    CaptureRotationControls.h
    ReactionCaptureDialog.h
    XtbDialog.cpp
    XtbDialog.h
    ScanResultsDialog.h
    ScanResultsDialog.cpp
    PlotWidget.cpp
    PlotWidget_Export.cpp
    AnalysisPanel.cpp
    analysispanel/AnalysisPanel_Geometry.cpp
    analysispanel/AnalysisPanel_Frequency.cpp
    analysispanel/AnalysisPanel_Coordinates.cpp
    analysispanel/AnalysisPanel_ScanIrc.cpp
    analysispanel/AnalysisPanel_Tddft.cpp
    OrbitalStudyWidget.cpp
    OrbitalStudyWidget.h
    analysispanel/AnalysisPanel_AtomsOrbitals.cpp
    StyleEditorDialog.cpp
    GeometryMeasureBar.cpp
    styleeditor/StyleEditor_Lighting.cpp
    styleeditor/StyleEditor_Atoms.cpp
    styleeditor/StyleEditor_Bonds.cpp
    styleeditor/StyleEditor_Isosurface.cpp
)
youngcraft_enable_warnings(youngcraft_ui)
target_link_libraries(youngcraft_ui
    PUBLIC youngcraft_headers Qt6::Widgets
    PRIVATE youngcraft_analysis youngcraft_animation youngcraft_compute youngcraft_export youngcraft_io youngcraft_viewport
)
