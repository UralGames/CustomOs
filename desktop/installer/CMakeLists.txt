qt_add_executable(meridian-installer main.cpp)
qt_add_qml_module(meridian-installer
    URI Meridian.Installer
    VERSION 1.0
    RESOURCE_PREFIX /
    SOURCES
        installerbackend.h installerbackend.cpp
    QML_FILES
        Main.qml
)
target_compile_definitions(meridian-installer PRIVATE MERIDIAN_LIBEXECDIR="${MERIDIAN_LIBEXECDIR}")
target_link_libraries(meridian-installer PRIVATE
    meridiancore meridiancoreplugin meridianstyle meridianstyleplugin
    Qt6::Quick Qt6::QuickControls2)
install(TARGETS meridian-installer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS customos-install DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
