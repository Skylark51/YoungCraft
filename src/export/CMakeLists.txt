add_library(youngcraft_export STATIC
    ImageExporter.cpp
    PowerPointVideo.cpp
    ReactionCapture.cpp
)
youngcraft_enable_warnings(youngcraft_export)
target_link_libraries(youngcraft_export
    PUBLIC youngcraft_headers youngcraft_model
    PRIVATE Qt6::Widgets
)
if(WIN32)
    target_link_libraries(youngcraft_export PRIVATE mfplat mfreadwrite mfuuid ole32)
endif()
