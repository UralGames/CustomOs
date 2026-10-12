# Meridian.Update: the update user interface. It talks to the CustomOS update
# service (customos-updated, from updater/) over D-Bus and is used by
# Settings › System › Updates and by the Meridian Update app.

qt_add_library(meridianupdate STATIC)
qt_add_qml_module(meridianupdate
    URI Meridian.Update
    VERSION 1.0
    RESOURCE_PREFIX /
    SOURCES
        updateclient.h updateclient.cpp
    QML_FILES
        UpdatesView.qml
        UpdateStatusCard.qml
        ReleaseNotesCard.qml
        UpdateDetailsCard.qml
        UpdateHistoryCard.qml
        InfoStrip.qml
        ConfirmDialog.qml
        Rule.qml
)
target_include_directories(meridianupdate PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(meridianupdate PUBLIC
    meridiancore Qt6::Core Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::DBus)

qt_add_executable(meridian-update main.cpp updatenotifier.h updatenotifier.cpp)
qt_add_qml_module(meridian-update
    URI Meridian.UpdateApp
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES
        Main.qml
)
target_link_libraries(meridian-update PRIVATE
    meridianupdate meridianupdateplugin
    meridiancore meridiancoreplugin meridianstyle meridianstyleplugin
    Qt6::Quick Qt6::QuickControls2 Qt6::DBus)
install(TARGETS meridian-update RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
