add_library(youngcraft_parsers STATIC
    NboParser.cpp
    OrcaParser.cpp
    GsmParser.cpp
    GaussianParser.cpp
    XYZParser.cpp
    CubeParser.cpp
    FchkParser.cpp
    StructureReader.cpp
    ${PROJECT_SOURCE_DIR}/third_party/gemmi/symmetry.cpp
)
youngcraft_enable_warnings(youngcraft_parsers)
target_link_libraries(youngcraft_parsers
    PUBLIC youngcraft_headers youngcraft_model
)
target_include_directories(youngcraft_parsers PRIVATE "${PROJECT_SOURCE_DIR}/third_party/gemmi/include")
if(MSVC)
    set_source_files_properties("${PROJECT_SOURCE_DIR}/third_party/gemmi/symmetry.cpp"
        PROPERTIES COMPILE_OPTIONS "/wd4244;/wd4267")
endif()

add_library(youngcraft_io STATIC
    FileDispatcher.cpp
)
youngcraft_enable_warnings(youngcraft_io)
target_link_libraries(youngcraft_io
    PUBLIC youngcraft_headers youngcraft_parsers
    PRIVATE youngcraft_compute
)
