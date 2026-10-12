# MeridianStyle: a Qt Quick Controls style built on QtQuick.Templates.
# Selected at start-up with QQuickStyle::setStyle("MeridianStyle"); controls
# not implemented here fall back to the Basic style.

qt_add_library(meridianstyle STATIC)
qt_add_qml_module(meridianstyle
    URI MeridianStyle
    VERSION 1.0
    RESOURCE_PREFIX /
    IMPORTS QtQuick.Controls.Basic
    QML_FILES
        ApplicationWindow.qml
        BusyIndicator.qml
        Button.qml
        CheckBox.qml
        ComboBox.qml
        Dialog.qml
        DialogButtonBox.qml
        Frame.qml
        ItemDelegate.qml
        Label.qml
        Menu.qml
        MenuItem.qml
        MenuSeparator.qml
        Page.qml
        Pane.qml
        Popup.qml
        ProgressBar.qml
        RadioButton.qml
        ScrollBar.qml
        ScrollView.qml
        Slider.qml
        Switch.qml
        TabBar.qml
        TabButton.qml
        TextArea.qml
        TextField.qml
        ToolBar.qml
        ToolButton.qml
        ToolSeparator.qml
        ToolTip.qml
        PopupShadow.qml
)
target_link_libraries(meridianstyle PUBLIC meridiancore Qt6::Quick Qt6::QuickControls2)
