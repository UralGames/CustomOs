################################################################################
#
# layer-shell-qt
#
################################################################################

LAYER_SHELL_QT_VERSION = 6.5.5
LAYER_SHELL_QT_SITE = https://download.kde.org/stable/plasma/$(LAYER_SHELL_QT_VERSION)
LAYER_SHELL_QT_SOURCE = layer-shell-qt-$(LAYER_SHELL_QT_VERSION).tar.xz
LAYER_SHELL_QT_LICENSE = LGPL-3.0-or-later
LAYER_SHELL_QT_LICENSE_FILES = LICENSES/LGPL-3.0-or-later.txt
LAYER_SHELL_QT_INSTALL_STAGING = YES
LAYER_SHELL_QT_SUPPORTS_IN_SOURCE_BUILD = NO
LAYER_SHELL_QT_DEPENDENCIES = \
	extra-cmake-modules \
	host-pkgconf \
	host-wayland \
	libxkbcommon \
	qt6base \
	qt6declarative \
	qt6wayland \
	wayland \
	wayland-protocols
LAYER_SHELL_QT_CONF_OPTS = \
	-DQT_HOST_PATH=$(HOST_DIR) \
	-DBUILD_TESTING=OFF \
	-DECM_DIR=$(STAGING_DIR)/usr/share/ECM/cmake

$(eval $(cmake-package))
