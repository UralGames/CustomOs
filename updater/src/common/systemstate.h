#pragma once

#include "version.h"

#include <QString>

// Facts about the running system that decide whether over-the-air updates
// are possible.
struct SystemState
{
    enum class Mode {
        AB,       // installed with the A/B layout, booted through the A/B GRUB menu
        Live,     // running from the live ISO (root file system in RAM)
        Legacy,   // installed by the CustomOS 0.1 installer (single root partition)
        Unknown
    };

    Mode mode = Mode::Unknown;
    QString osName;          // NAME
    QString prettyName;      // PRETTY_NAME
    Version version;         // VERSION_ID
    QString versionText;     // VERSION_ID as written
    QString buildId;         // BUILD_ID
    QString bootedSlot;      // "A" or "B" (rauc.slot= on the kernel command line)

    static SystemState probe();
    // For tests: explicit file locations.
    static SystemState probeFrom(const QString &osReleasePath, const QString &cmdlinePath,
                                 const QString &liveMarkerPath);

    static QString modeName(Mode m);
};
