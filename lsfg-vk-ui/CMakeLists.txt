find_package(Qt6 REQUIRED COMPONENTS Quick)

set(UI_SOURCES
    "src/backend.cpp"
    "src/main.cpp"
    "src/utils.cpp")

set(UI_RESOURCES
    "rsc/panes/CenteredDialog.qml"
    "rsc/panes/Group.qml"
    "rsc/panes/GroupEntry.qml"
    "rsc/panes/Pane.qml"
    "rsc/panes/ProfileList.qml"
    "rsc/widgets/FileEdit.qml"
    "rsc/widgets/FlowSlider.qml"
    "rsc/UI.qml")

qt_add_executable(lsfg-vk-ui ${UI_SOURCES})

qt_add_resources(lsfg-vk-ui RESOURCES
    PREFIX "/"
    FILES ${UI_RESOURCES})

set_target_properties(lsfg-vk-ui PROPERTIES
    AUTOMOC ON
    AUTOUIC ON)

target_compile_options(lsfg-vk-ui PRIVATE
    -Wno-ctad-maybe-unsupported
    -Wno-unsafe-buffer-usage-in-libc-call
    -Wno-global-constructors
    -Wno-unsafe-buffer-usage)

target_link_libraries(lsfg-vk-ui
    PRIVATE lsfg-vk-common
    PRIVATE lsfg-vk-backend
    PRIVATE Qt6::Quick)
