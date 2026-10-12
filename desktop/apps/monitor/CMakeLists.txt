qt_add_executable(meridian-monitor main.cpp)
qt_add_qml_module(meridian-monitor
    URI Meridian.Monitor
    VERSION 1.0
    RESOURCE_PREFIX /
    SOURCES
        stats.h stats.cpp
    QML_FILES
        Main.qml
        Graph.qml
        ColumnHeader.qml
)
target_link_libraries(meridian-monitor PRIVATE
    meridiancore meridiancoreplugin meridianstyle meridianstyleplugin
    Qt6::Quick Qt6::QuickControls2)
install(TARGETS meridian-monitor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
