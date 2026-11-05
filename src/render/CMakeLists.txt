add_library(youngcraft_isosurface STATIC
    IsosurfaceBuilder.cpp
)
youngcraft_enable_warnings(youngcraft_isosurface)
target_link_libraries(youngcraft_isosurface
    PUBLIC youngcraft_headers youngcraft_parsers
)

add_library(youngcraft_viewport STATIC
    MoleculeView.cpp
    moleculeview/MoleculeView_State.cpp
    moleculeview/MoleculeView_Camera.cpp
    moleculeview/MoleculeView_GL.cpp
    moleculeview/MoleculeView_Surface.cpp
    moleculeview/MoleculeView_Shadows.cpp
    moleculeview/MoleculeView_AtomsBonds.cpp
    moleculeview/MoleculeView_Scene.cpp
    moleculeview/MoleculeView_Overlays.cpp
    moleculeview/MoleculeView_Interaction.cpp
)
youngcraft_enable_warnings(youngcraft_viewport)
target_link_libraries(youngcraft_viewport
    PUBLIC youngcraft_headers youngcraft_isosurface youngcraft_settings Qt6::OpenGL Qt6::OpenGLWidgets
)
