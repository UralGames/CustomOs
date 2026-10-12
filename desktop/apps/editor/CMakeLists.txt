qt_add_executable(meridian-editor main.cpp)
qt_add_qml_module(meridian-editor
    URI Meridian.Editor
    VERSION 1.0
    RESOURCE_PREFIX /
    SOURCES
        documentio.h documentio.cpp
    QML_FILES
        Main.qml
)
target_link_libraries(meridian-editor PRIVATE
    meridiancore meridiancoreplugin meridianstyle meridianstyleplugin
    Qt6::Quick Qt6::QuickControls2)
install(TARGETS meridian-editor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
