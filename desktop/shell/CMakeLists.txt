find_package(Qt6 REQUIRED COMPONENTS WaylandClient Gui)
find_package(LayerShellQt CONFIG QUIET)

qt_add_executable(meridian-shell main.cpp)

qt6_generate_wayland_protocol_client_sources(meridian-shell FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../protocols/wlr-foreign-toplevel-management-unstable-v1.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/../protocols/ext-workspace-v1.xml
)

set_source_files_properties(ShellState.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(meridian-shell
    URI Meridian.Shell
    VERSION 1.0
    RESOURCE_PREFIX /
    SOURCES
        layershell.h layershell.cpp
        toplevelmodel.h toplevelmodel.cpp
        workspacemodel.h workspacemodel.cpp
        shellipc.h shellipc.cpp
        notificationserver.h notificationserver.cpp
        session.h session.cpp
    QML_FILES
        ShellState.qml
        Main.qml
        BackgroundWindow.qml
        TaskbarWindow.qml
        OverlayWindow.qml
        ToastWindow.qml
        OsdWindow.qml
        TooltipWindow.qml
        Wallpaper.qml
        Taskbar.qml
        TaskbarIconButton.qml
        TaskbarButton.qml
        TrayButton.qml
        Flyout.qml
        Launcher.qml
        LauncherTile.qml
        QuickSettings.qml
        QuickTile.qml
        CalendarFlyout.qml
        NotificationCard.qml
        Toasts.qml
        Osd.qml
        TaskView.qml
        ContextMenuPanel.qml
)

if(LayerShellQt_FOUND)
    target_compile_definitions(meridian-shell PRIVATE MERIDIAN_HAVE_LAYERSHELLQT)
    target_link_libraries(meridian-shell PRIVATE LayerShellQt::Interface)
else()
    message(WARNING "LayerShellQt not found: the shell will open ordinary windows (development only)")
endif()

target_link_libraries(meridian-shell PRIVATE
    meridiancore meridiancoreplugin
    meridianstyle meridianstyleplugin
    Qt6::Quick Qt6::QuickControls2 Qt6::DBus Qt6::Network Qt6::WaylandClient Wayland::Client)
if(Qt6_VERSION VERSION_LESS 6.5)
    # Qt < 6.5 exposes the wl_seat only through the QPA native interface
    # (needs the Qt GUI private headers).
    target_link_libraries(meridian-shell PRIVATE Qt6::GuiPrivate)
endif()

install(TARGETS meridian-shell RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
