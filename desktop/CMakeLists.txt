cmake_minimum_required(VERSION 3.21)
project(meridian VERSION 1.0.0 LANGUAGES C CXX)

# Meridian desktop: a Qt 6 / QML desktop shell for the labwc Wayland
# compositor, plus a set of applications. Builds against Qt >= 6.4
# (developed against 6.4; the OS image uses 6.9).

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(GNUInstallDirs)

option(MERIDIAN_BUILD_SHELL     "Build the compositor shell"          ON)
option(MERIDIAN_BUILD_APPS      "Build the desktop applications"      ON)
option(MERIDIAN_BUILD_LOGIN     "Build the login service and greeter" ON)
option(MERIDIAN_BUILD_INSTALLER "Build the graphical installer"       ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Svg DBus Network)
qt_standard_project_setup()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)

set(MERIDIAN_DATADIR "${CMAKE_INSTALL_FULL_DATADIR}/meridian")
set(MERIDIAN_LIBEXECDIR "${CMAKE_INSTALL_FULL_LIBEXECDIR}")

# Shared compile definitions: where installed data lives, and the source
# asset directory so binaries run straight from the build tree during
# development (MERIDIAN_DATA_DIR in the environment overrides both).
add_compile_definitions(
    MERIDIAN_VERSION="${PROJECT_VERSION}"
    MERIDIAN_INSTALL_DATADIR="${MERIDIAN_DATADIR}"
    MERIDIAN_SOURCE_ASSETDIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
    MERIDIAN_LIBEXECDIR="${MERIDIAN_LIBEXECDIR}"
)

add_subdirectory(core)
add_subdirectory(style)

if(MERIDIAN_BUILD_SHELL)
    add_subdirectory(shell)
endif()
if(MERIDIAN_BUILD_APPS)
    foreach(app files settings monitor editor)
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/apps/${app}/CMakeLists.txt)
            add_subdirectory(apps/${app})
        endif()
    endforeach()
endif()
if(MERIDIAN_BUILD_LOGIN)
    add_subdirectory(login)
endif()
if(MERIDIAN_BUILD_INSTALLER)
    add_subdirectory(installer)
endif()

# ---- data -------------------------------------------------------------------
install(DIRECTORY assets/wallpapers/ DESTINATION ${CMAKE_INSTALL_DATADIR}/meridian/wallpapers
        FILES_MATCHING PATTERN "*.jpg")
install(DIRECTORY assets/images/ DESTINATION ${CMAKE_INSTALL_DATADIR}/meridian/images
        FILES_MATCHING PATTERN "*.png")
install(DIRECTORY assets/fonts/ DESTINATION ${CMAKE_INSTALL_DATADIR}/fonts/meridian)
install(DIRECTORY assets/icons/apps/ DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
        FILES_MATCHING PATTERN "*.svg")
install(DIRECTORY session/applications/ DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
        FILES_MATCHING PATTERN "*.desktop")
install(DIRECTORY session/themes/ DESTINATION ${CMAKE_INSTALL_DATADIR}/themes
        PATTERN "*.py" EXCLUDE)
install(PROGRAMS session/meridian-session session/meridian-terminal
        DESTINATION ${CMAKE_INSTALL_BINDIR})

# labwc: the template the shell renders into ~/.config/labwc/rc.xml, and a
# system-wide copy rendered with default settings for first start.
install(FILES session/labwc/rc.xml.in DESTINATION ${CMAKE_INSTALL_DATADIR}/meridian/labwc)
set(THEME "Meridian-Light")
set(CORNER_RADIUS 10)
set(DESKTOP_NAMES "<name>Desktop 1</name>\n      <name>Desktop 2</name>\n      <name>Desktop 3</name>\n      <name>Desktop 4</name>")
string(REPLACE "\\n" "\n" DESKTOP_NAMES "${DESKTOP_NAMES}")
set(REPEAT_RATE 30)
set(REPEAT_DELAY 400)
set(NATURAL_SCROLL no)
set(LEFT_HANDED no)
set(POINTER_SPEED 0.00)
set(TAP yes)
configure_file(session/labwc/rc.xml.in ${CMAKE_BINARY_DIR}/labwc/rc.xml @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/labwc/rc.xml session/labwc/menu.xml session/labwc/environment
        DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg/labwc)
install(PROGRAMS session/labwc/autostart DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg/labwc)
install(FILES session/foot.ini DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg/foot)
install(FILES session/swaylock/config DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/swaylock)
