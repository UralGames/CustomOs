import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Meridian.Core
import Meridian.Settings
import Meridian.Update

// Settings › System › Updates
SettingsPage {
    id: page
    title: qsTr("Updates")
    parentTitle: qsTr("System")
    parentPage: "system"
    subtitle: qsTr("New versions of %1 come from GitHub and are verified before they are installed.")
              .arg(Updates.status.osName || "CustomOS")

    Component.onCompleted: Updates.refresh()

    UpdatesView {
        Layout.fillWidth: true
        notify: page.notify
    }
}
