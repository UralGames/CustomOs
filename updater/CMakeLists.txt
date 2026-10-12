cmake_minimum_required(VERSION 3.21)
project(customos-updater VERSION 0.2.0 LANGUAGES CXX)

# CustomOS over-the-air updater: a root D-Bus service (customos-updated) that
# finds releases on GitHub, downloads and verifies RAUC bundles and installs
# them into the inactive A/B slot, plus a command-line client
# (customos-update). The Meridian Update UI (desktop/update) talks to the
# service over D-Bus. Builds against Qt >= 6.4 (the OS image uses 6.9).

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
include(GNUInstallDirs)

option(CUSTOMOS_UPDATER_BUILD_TESTS "Build the unit tests" OFF)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Network DBus)

# Build-time defaults; /etc/customos/update.conf overrides them at run time.
set(CUSTOMOS_UPDATE_REPOSITORY "" CACHE STRING "Default GitHub repository (owner/name or URL)")
set(CUSTOMOS_UPDATE_COMPATIBLE "customos-x86_64-ab" CACHE STRING "RAUC compatible string of this system")
set(CUSTOMOS_UPDATE_CHANNEL "stable" CACHE STRING "Default release channel")

add_library(customos-update-core STATIC
    src/common/version.h src/common/version.cpp
    src/common/manifest.h src/common/manifest.cpp
    src/common/config.h src/common/config.cpp
    src/common/systemstate.h src/common/systemstate.cpp
    src/common/statestore.h src/common/statestore.cpp
    src/common/errors.h src/common/errors.cpp
    src/common/githubclient.h src/common/githubclient.cpp
    src/common/downloader.h src/common/downloader.cpp
    src/common/raucclient.h src/common/raucclient.cpp
    src/common/updateservice.h src/common/updateservice.cpp
    src/common/dbusadaptor.h src/common/dbusadaptor.cpp
    src/common/dbusnames.h
)
target_include_directories(customos-update-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_compile_definitions(customos-update-core PUBLIC
    CUSTOMOS_UPDATER_VERSION="${PROJECT_VERSION}"
    CUSTOMOS_SYSCONFDIR="${CMAKE_INSTALL_FULL_SYSCONFDIR}"
    CUSTOMOS_DEFAULT_REPOSITORY="${CUSTOMOS_UPDATE_REPOSITORY}"
    CUSTOMOS_DEFAULT_COMPATIBLE="${CUSTOMOS_UPDATE_COMPATIBLE}"
    CUSTOMOS_DEFAULT_CHANNEL="${CUSTOMOS_UPDATE_CHANNEL}"
)
target_compile_definitions(customos-update-core PRIVATE QT_NO_CAST_FROM_ASCII)
target_link_libraries(customos-update-core PUBLIC Qt6::Core Qt6::Network Qt6::DBus)

add_executable(customos-updated src/daemon/main.cpp)
target_link_libraries(customos-updated PRIVATE customos-update-core)
target_compile_definitions(customos-updated PRIVATE QT_NO_CAST_FROM_ASCII)

add_executable(customos-update src/cli/main.cpp)
target_link_libraries(customos-update PRIVATE customos-update-core)
target_compile_definitions(customos-update PRIVATE QT_NO_CAST_FROM_ASCII)

install(TARGETS customos-updated RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
install(TARGETS customos-update RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---- system integration -------------------------------------------------------
install(FILES data/org.customos.Update1.conf DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system.d)
configure_file(data/org.customos.Update1.service.in org.customos.Update1.service @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.customos.Update1.service
        DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system-services)
configure_file(data/customos-updated.service.in customos-updated.service @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/customos-updated.service
              boot/customos-health-check.service
              boot/customos-ota-setup.service
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/system)
configure_file(data/update.conf.in update.conf @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/update.conf
        DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/customos)

# Boot integration (A/B slots, persistent data, health check) and installer.
set(CUSTOMOS_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib/customos)
install(PROGRAMS boot/customos-preinit
                 boot/customos-health-check
                 boot/customos-ota-setup
                 boot/customos-rauc-hook
        DESTINATION ${CUSTOMOS_LIBDIR})
install(FILES boot/ota-lib.sh boot/grub-ab.cfg boot/persistent-etc.list
        DESTINATION ${CUSTOMOS_LIBDIR})
# RAUC calls its handlers without arguments: one name per phase.
install(CODE "
    foreach(phase pre-install post-install)
        execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink customos-rauc-hook
                        \$ENV{DESTDIR}${CUSTOMOS_LIBDIR}/customos-rauc-\${phase})
    endforeach()")
# The slot devices are filled in at boot by customos-preinit.
set(COMPATIBLE ${CUSTOMOS_UPDATE_COMPATIBLE})
set(SLOT_A_DEVICE "@SLOT_A_DEVICE@")
set(SLOT_B_DEVICE "@SLOT_B_DEVICE@")
configure_file(boot/rauc-system.conf rauc-system.conf @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rauc-system.conf DESTINATION ${CUSTOMOS_LIBDIR})
install(FILES boot/health.conf DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/customos)
install(PROGRAMS installer/customos-install-ab DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})

if(CUSTOMOS_UPDATER_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    foreach(t version manifest github statestore service)
        add_executable(tst_${t} tests/tst_${t}.cpp tests/testutil.h)
        target_include_directories(tst_${t} PRIVATE tests)
        target_link_libraries(tst_${t} PRIVATE customos-update-core Qt6::Test)
        add_test(NAME ${t} COMMAND tst_${t})
    endforeach()
    # The service test talks to a fake RAUC on a private session bus.
    set_tests_properties(service PROPERTIES ENVIRONMENT "CUSTOMOS_UPDATE_TEST=1")
endif()
