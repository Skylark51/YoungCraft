add_library(youngcraft_animation STATIC
    AnimationController.cpp
    MechanismTimeline.cpp
)
youngcraft_enable_warnings(youngcraft_animation)
target_link_libraries(youngcraft_animation
    PUBLIC youngcraft_headers youngcraft_model
)
