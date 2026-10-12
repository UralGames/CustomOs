# Meridian.Core: C++ services and shared QML (theme, icons) used by the shell
# and every application. Built as a static QML module and linked into each
# executable, so nothing depends on runtime QML import paths.

qt_add_library(meridiancore STATIC)
# Must be set before qt_add_qml_module so the qmldir declares the singleton.
set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(meridiancore
    URI Meridian.Core
    VERSION 1.0
    RESOURCE_PREFIX /
    SOURCES
        settings.h settings.cpp
        desktopentry.h desktopentry.cpp
        appmodel.h appmodel.cpp
        iconprovider.h iconprovider.cpp
        sys.h sys.cpp
        systeminfo.h systeminfo.cpp
        audiocontrol.h audiocontrol.cpp
        networkcontrol.h networkcontrol.cpp
        displaycontrol.h displaycontrol.cpp
        meridian.h meridian.cpp
    QML_FILES
        Theme.qml
        Icon.qml
        AppIcon.qml
        Heading.qml
        SectionCard.qml
        FileChooser.qml
)

get_filename_component(MERIDIAN_ASSETS ${CMAKE_CURRENT_SOURCE_DIR}/../assets ABSOLUTE)
file(GLOB MERIDIAN_SYMBOLIC_ICONS ${MERIDIAN_ASSETS}/icons/symbolic/*.svg)
file(GLOB MERIDIAN_APP_ICONS ${MERIDIAN_ASSETS}/icons/apps/*.svg)
qt_add_resources(meridiancore "meridian_icons"
    PREFIX "/"
    BASE ${MERIDIAN_ASSETS}
    FILES ${MERIDIAN_SYMBOLIC_ICONS} ${MERIDIAN_APP_ICONS}
          ${MERIDIAN_ASSETS}/images/shadow-window.png
          ${MERIDIAN_ASSETS}/images/shadow-popup.png
)

target_include_directories(meridiancore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(meridiancore PUBLIC
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Svg)
