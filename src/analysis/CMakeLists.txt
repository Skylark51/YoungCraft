add_library(youngcraft_analysis STATIC
    AnalysisEngine.cpp
)
youngcraft_enable_warnings(youngcraft_analysis)
target_link_libraries(youngcraft_analysis
    PUBLIC youngcraft_headers youngcraft_model
)
