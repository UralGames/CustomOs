################################################################################
#
# meridian-desktop
#
################################################################################

MERIDIAN_DESKTOP_VERSION = 1.0.0
MERIDIAN_DESKTOP_SITE = $(BR2_EXTERNAL_CUSTOMOS_PATH)/../desktop
MERIDIAN_DESKTOP_SITE_METHOD = local
MERIDIAN_DESKTOP_LICENSE = MIT (code, artwork), OFL-1.1 (Inter, JetBrains Mono fonts)
MERIDIAN_DESKTOP_LICENSE_FILES = LICENSE assets/fonts/LICENSE-Inter.txt assets/fonts/LICENSE-JetBrainsMono.txt
MERIDIAN_DESKTOP_SUPPORTS_IN_SOURCE_BUILD = NO
MERIDIAN_DESKTOP_CMAKE_BACKEND = ninja
MERIDIAN_DESKTOP_DEPENDENCIES = \
	host-pkgconf \
	host-wayland \
	host-qt6base \
	host-qt6declarative \
	host-qt6wayland \
	layer-shell-qt \
	linux-pam \
	qt6base \
	qt6declarative \
	qt6svg \
	qt6wayland \
	wayland

MERIDIAN_DESKTOP_CONF_OPTS = \
	-DQT_HOST_PATH=$(HOST_DIR) \
	-DMERIDIAN_BUILD_SHELL=ON \
	-DMERIDIAN_BUILD_APPS=ON \
	-DMERIDIAN_BUILD_LOGIN=ON \
	-DMERIDIAN_BUILD_INSTALLER=ON

# Start the graphical sign-in at boot (graphical.target is the default).
define MERIDIAN_DESKTOP_ENABLE_LOGIN
	mkdir -p $(TARGET_DIR)/etc/systemd/system/graphical.target.wants
	ln -sf /usr/lib/systemd/system/meridian-login.service \
		$(TARGET_DIR)/etc/systemd/system/graphical.target.wants/meridian-login.service
	ln -sf /usr/lib/systemd/system/meridian-login.service \
		$(TARGET_DIR)/etc/systemd/system/display-manager.service
endef
MERIDIAN_DESKTOP_POST_INSTALL_TARGET_HOOKS += MERIDIAN_DESKTOP_ENABLE_LOGIN

$(eval $(cmake-package))
