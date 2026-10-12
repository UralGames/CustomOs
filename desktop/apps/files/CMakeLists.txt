qt_add_executable(meridian-files main.cpp)
qt_add_qml_module(meridian-files
    URI Meridian.Files
    VERSION 1.0
    RESOURCE_PREFIX /
    SOURCES
        dirmodel.h dirmodel.cpp
        fileops.h fileops.cpp
    QML_FILES
        Main.qml
        PathBar.qml
        SortHeader.qml
        FileIcon.qml
        FileRow.qml
        FileTile.qml
        NameDialog.qml
        PropertiesDialog.qml
        OpenWithDialog.qml
)
target_link_libraries(meridian-files PRIVATE
    meridiancore meridiancoreplugin meridianstyle meridianstyleplugin
    Qt6::Quick Qt6::QuickControls2)
install(TARGETS meridian-files RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
