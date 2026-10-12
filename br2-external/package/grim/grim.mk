################################################################################
#
# grim
#
################################################################################

GRIM_VERSION = 1.4.1
GRIM_SITE = https://gitlab.freedesktop.org/emersion/grim/-/archive/v$(GRIM_VERSION)
GRIM_SOURCE = grim-v$(GRIM_VERSION).tar.gz
GRIM_LICENSE = MIT
GRIM_LICENSE_FILES = LICENSE
GRIM_DEPENDENCIES = host-pkgconf host-wayland libpng pixman wayland wayland-protocols
GRIM_CONF_OPTS = \
	-Djpeg=disabled \
	-Dman-pages=disabled \
	-Dzsh-completions=false \
	-Dbash-completions=false \
	-Dfish-completions=false

$(eval $(meson-package))
