add_library(youngcraft_model STATIC
    StructureTools.cpp
    MoleculeEditController.cpp
    FragmentLibrary.cpp
    LigandReplacement.cpp
    Molecule.cpp
    GeometryEditor.cpp
)
youngcraft_enable_warnings(youngcraft_model)
target_link_libraries(youngcraft_model
    PUBLIC youngcraft_headers Qt6::Gui
)
