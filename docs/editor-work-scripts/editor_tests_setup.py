from pathlib import Path
p=Path('CMakeLists.txt');s=p.read_text(encoding='utf-8');s+='''
if(BUILD_TESTING)
    qt_add_executable(test_editor tests/editor/test_editor.cpp resources/fragments.qrc)
    target_link_libraries(test_editor PRIVATE youngcraft_core Qt6::Test)
    add_test(NAME molecular_editor COMMAND test_editor)
    if(WIN32)
        add_custom_command(TARGET test_editor POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:test_editor> $<TARGET_FILE_DIR:test_editor> COMMAND_EXPAND_LISTS VERBATIM)
    endif()
endif()
''';p.write_text(s,encoding='utf-8')
