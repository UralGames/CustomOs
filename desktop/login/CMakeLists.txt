# meridian-login (root service, plain C++ + PAM) and meridian-greeter (Qt UI).

find_library(PAM_LIBRARY pam REQUIRED)
find_path(PAM_INCLUDE_DIR security/pam_appl.h REQUIRED)

add_executable(meridian-login meridian-login.cpp)
target_include_directories(meridian-login PRIVATE ${PAM_INCLUDE_DIR})
target_link_libraries(meridian-login PRIVATE ${PAM_LIBRARY})
target_compile_definitions(meridian-login PRIVATE MERIDIAN_LIBEXECDIR="${MERIDIAN_LIBEXECDIR}")
set_target_properties(meridian-login PROPERTIES AUTOMOC OFF)

qt_add_executable(meridian-greeter greeter.cpp)
qt_add_qml_module(meridian-greeter
    URI Meridian.Greeter
    VERSION 1.0
    RESOURCE_PREFIX /
    SOURCES
        greeterbackend.h greeterbackend.cpp
    QML_FILES
        Main.qml
)
target_link_libraries(meridian-greeter PRIVATE
    meridiancore meridiancoreplugin meridianstyle meridianstyleplugin
    Qt6::Quick Qt6::QuickControls2)

install(TARGETS meridian-login meridian-greeter RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
install(FILES meridian-login.service DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/system)
install(FILES pam/meridian pam/meridian-autologin pam/meridian-lock
        DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/pam.d)
