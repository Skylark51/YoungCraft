add_library(youngcraft_settings STATIC
    StyleManager.cpp
)
youngcraft_enable_warnings(youngcraft_settings)
target_link_libraries(youngcraft_settings
    PUBLIC youngcraft_headers Qt6::Gui
    PRIVATE youngcraft_model
)
