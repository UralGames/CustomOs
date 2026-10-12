#!/usr/bin/env python3
"""Stand-in for customos-updated on the D-Bus *session* bus, for working on
and screenshotting the Meridian Update UI without a real update.

    dbus-run-session -- sh -c '
        tests/ui/mock-update-service.py available &
        CUSTOMOS_UPDATE_BUS=session meridian-update'

The scenario fixes the status the UI sees; buttons are accepted and logged.
"""
import json
import sys
import time

import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

NOW = int(time.time())

NOTES = """**Faster start-up and a smoother desktop.**

- The desktop appears about 30% sooner after signing in.
- Wi-Fi reconnects automatically after the computer wakes up.
- New in Settings: *Updates*, with release notes and rollback.
- Security fixes for OpenSSL, the Linux kernel and Qt.

See the full list of changes on the [release page](https://github.com/UralGames/CustomOs/releases).
"""

BASE = {
    "state": "idle", "busy": False, "mode": "ab", "supported": True, "unsupportedMessage": "",
    "osName": "CustomOS", "prettyName": "CustomOS 0.2.0 (Meridian)", "currentVersion": "0.2.0",
    "buildId": "3f9c2a1b7e4d", "bootedSlot": "A", "otherSlot": "B", "otherSlotVersion": "",
    "rollbackAvailable": False, "repository": "UralGames/CustomOs", "channel": "stable",
    "automaticCheck": True, "updaterVersion": "0.2.0", "updateAvailable": False,
    "availableVersion": "", "releaseName": "", "releaseNotes": "", "releaseUrl": "",
    "releaseDate": 0, "downloadSize": 0, "rebootNeeded": True, "downloaded": False, "canInstall": False,
    "progress": -1, "progressText": "", "bytesDownloaded": 0, "downloadRate": 0.0,
    "errorCode": "none", "errorMessage": "", "errorDetail": "", "lastCheck": NOW - 3600 * 2,
    "lastCheckError": "", "lastSuccessVersion": "", "lastSuccessTime": 0,
    "rebootRequired": False, "pendingKind": "", "pendingVersion": "",
}
AVAILABLE = {
    "updateAvailable": True, "availableVersion": "0.3.0", "releaseName": "CustomOS 0.3.0",
    "releaseNotes": NOTES, "releaseUrl": "https://github.com/UralGames/CustomOs/releases/tag/v0.3.0",
    "releaseDate": NOW - 86400 * 2, "downloadSize": 431 * 1024 * 1024, "canInstall": True,
}
LIVE_MSG = "You're running CustomOS from the live image. Install CustomOS to a disk to receive updates."

SCENARIOS = {
    "idle": {"lastCheck": 0},
    "checking": {"state": "checking", "busy": True},
    "up-to-date": {"state": "up-to-date", "lastSuccessVersion": "0.2.0", "lastSuccessTime": NOW - 86400 * 9,
                   "bootedSlot": "B", "otherSlot": "A", "otherSlotVersion": "0.1.5", "rollbackAvailable": True},
    "available": dict(AVAILABLE, state="update-available"),
    "downloading": dict(AVAILABLE, state="downloading", busy=True, canInstall=False, progress=38,
                        progressText="Downloading", bytesDownloaded=int(431 * 1024 * 1024 * 0.38),
                        downloadRate=6.2 * 1024 * 1024),
    "verifying": dict(AVAILABLE, state="verifying", busy=True, canInstall=False, progressText="Checking the signature"),
    "installing": dict(AVAILABLE, state="installing", busy=True, canInstall=False, progress=64,
                       progressText="Copying image to rootfs.1", downloaded=True),
    "reboot": dict(BASE, state="reboot-required", rebootRequired=True, pendingKind="update", pendingVersion="0.3.0"),
    "error-offline": {"state": "error", "errorCode": "offline",
                      "errorMessage": "CustomOS couldn't reach GitHub to check for updates. Check that you're "
                                      "connected to the internet, then try again.",
                      "errorDetail": "Host api.github.com not found (api.github.com)"},
    "error-signature": dict(AVAILABLE, state="error", errorCode="signature-invalid",
                            errorMessage="The update isn't signed by a trusted CustomOS key, so it was not "
                                         "installed. Your system was not changed.",
                            errorDetail="signature verification failed: certificate verify failed"),
    "rolled-back": {"state": "error", "errorCode": "rolled-back",
                    "errorMessage": "CustomOS 0.3.0 didn't start correctly, so your computer went back to the "
                                    "previous version. Nothing was lost.",
                    "errorDetail": "did not start: meridian-login.service"},
    "live": dict(AVAILABLE, state="update-available", mode="live", supported=False, canInstall=False,
                 unsupportedMessage=LIVE_MSG, bootedSlot=""),
}

HISTORY = [
    {"time": "2026-09-21T10:15:40Z", "event": "update", "result": "success", "from_version": "0.1.5",
     "to_version": "0.2.0", "message": "Updated and started successfully"},
    {"time": "2026-10-03T18:02:11Z", "event": "install", "result": "failed", "from_version": "0.2.0",
     "to_version": "0.2.1", "message": "Failed writing image: No space left on device"},
]


def typed(status):
    out = {}
    for k, v in status.items():
        if isinstance(v, bool):
            out[k] = dbus.Boolean(v)
        elif isinstance(v, int):
            out[k] = dbus.UInt64(v) if k in ("downloadSize", "bytesDownloaded") else dbus.Int64(v)
        elif isinstance(v, float):
            out[k] = dbus.Double(v)
        else:
            out[k] = dbus.String(v)
    return dbus.Dictionary(out, signature="sv")


class Service(dbus.service.Object):
    def __init__(self, bus, scenario):
        super().__init__(bus, "/org/customos/Update1")
        self.status = dict(BASE, **SCENARIOS[scenario])

    @dbus.service.method("org.customos.Update1", out_signature="a{sv}")
    def GetStatus(self):
        return typed(self.status)

    @dbus.service.method("org.customos.Update1", out_signature="s")
    def GetHistory(self):
        return json.dumps(HISTORY)

    def _log(self, name):
        print(f"mock-update-service: {name}()", file=sys.stderr, flush=True)

    @dbus.service.method("org.customos.Update1")
    def CheckForUpdates(self): self._log("CheckForUpdates")

    @dbus.service.method("org.customos.Update1")
    def Download(self): self._log("Download")

    @dbus.service.method("org.customos.Update1")
    def Install(self): self._log("Install")

    @dbus.service.method("org.customos.Update1")
    def Cancel(self): self._log("Cancel")

    @dbus.service.method("org.customos.Update1")
    def Reboot(self): self._log("Reboot")

    @dbus.service.method("org.customos.Update1")
    def Rollback(self): self._log("Rollback")

    @dbus.service.method("org.customos.Update1")
    def DismissError(self): self._log("DismissError")

    @dbus.service.signal("org.customos.Update1", signature="a{sv}")
    def StatusChanged(self, status):
        pass


def main():
    scenario = sys.argv[1] if len(sys.argv) > 1 else "available"
    if scenario not in SCENARIOS:
        sys.exit(f"unknown scenario {scenario}; one of: {', '.join(SCENARIOS)}")
    DBusGMainLoop(set_as_default=True)
    bus = dbus.SessionBus()
    name = dbus.service.BusName("org.customos.Update1", bus)  # noqa: F841 (keeps the name)
    Service(bus, scenario)
    GLib.MainLoop().run()


if __name__ == "__main__":
    main()
