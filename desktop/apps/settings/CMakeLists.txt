qt_add_executable(meridian-settings main.cpp)
qt_add_qml_module(meridian-settings
    URI Meridian.Settings
    VERSION 1.0
    RESOURCE_PREFIX /
    SOURCES
        systemactions.h systemactions.cpp
    QML_FILES
        Main.qml
        SettingsPage.qml
        SettingRow.qml
        Divider.qml
        SpinBoxLike.qml
        SystemPage.qml
        DisplayPage.qml
        SoundPage.qml
        NetworkPage.qml
        AppearancePage.qml
        TaskbarPage.qml
        MultitaskingPage.qml
        InputPage.qml
        NotificationsPage.qml
        TimePage.qml
        UserPage.qml
)
target_link_libraries(meridian-settings PRIVATE
    meridiancore meridiancoreplugin meridianstyle meridianstyleplugin
    Qt6::Quick Qt6::QuickControls2)
install(TARGETS meridian-settings RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
