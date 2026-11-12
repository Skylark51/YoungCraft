add_library(youngcraft_compute STATIC
    GaussianConnection.cpp
    GaussianConnection.h
    GaussianRemote.cpp
    GaussianRemote.h
    GaussianTools.cpp
    GaussianTools.h
    SshPasswordBroker.cpp
    SshPasswordBroker.h
    XtbRunner.cpp
    XtbRunner.h
)
youngcraft_enable_warnings(youngcraft_compute)
target_link_libraries(youngcraft_compute
    PUBLIC youngcraft_headers youngcraft_parsers Qt6::Network Qt6::Widgets
)
