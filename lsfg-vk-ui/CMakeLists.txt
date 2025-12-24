find_package(Qt6 REQUIRED COMPONENTS Quick)

set(UI_SOURCES
    "src/backend.cpp"
    "src/main.cpp"
    "src/utils.cpp")

set(UI_RESOURCES
    "rsc/dialogs/CenteredDialog.qml"
    "rsc/dialogs/LargeDialog.qml"
    "rsc/panes/Group.qml"
    "rsc/panes/GroupEntry.qml"
    "rsc/panes/Pane.qml"
    "rsc/widgets/FileEdit.qml"
    "rsc/widgets/FlowSlider.qml"
    "rsc/widgets/List.qml"
    "rsc/gay.pancake.lsfg-vk-ui.png"
    "rsc/UI.qml")

qt_add_executable(lsfg-vk-ui ${UI_SOURCES})

qt_add_resources(lsfg-vk-ui RESOURCES
    PREFIX "/"
    FILES ${UI_RESOURCES})

set_target_properties(lsfg-vk-ui PROPERTIES
    AUTOMOC ON
    AUTOUIC ON)

target_compile_options(lsfg-vk-ui PRIVATE
    -Wno-unknown-warning-option
    -Wno-ctad-maybe-unsupported
    -Wno-unsafe-buffer-usage-in-libc-call
    -Wno-global-constructors
    -Wno-unsafe-buffer-usage)

target_link_libraries(lsfg-vk-ui
    PRIVATE lsfg-vk-common
    PRIVATE lsfg-vk-backend
    PRIVATE Qt6::Quick)

install(TARGETS lsfg-vk-ui
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
if(LSFGVK_INSTALL_XDG_FILES)
    install(FILES "rsc/gay.pancake.lsfg-vk-ui.png"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps")
    install(FILES "rsc/gay.pancake.lsfg-vk-ui.desktop"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
endif()
