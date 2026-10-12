################################################################################
#
# labwc
#
################################################################################

LABWC_VERSION = 0.9.8
LABWC_SITE = $(call github,labwc,labwc,$(LABWC_VERSION))
LABWC_LICENSE = GPL-2.0
LABWC_LICENSE_FILES = LICENSE
LABWC_DEPENDENCIES = \
	host-pkgconf \
	host-wayland \
	cairo \
	libdrm \
	libglib2 \
	libinput \
	libpng \
	librsvg \
	libsfdo \
	libxkbcommon \
	libxml2 \
	pango \
	pixman \
	wayland \
	wayland-protocols \
	wlroots

LABWC_CONF_OPTS = \
	-Dxwayland=disabled \
	-Dsvg=enabled \
	-Dicon=enabled \
	-Dman-pages=disabled \
	-Dnls=disabled \
	-Dtest=disabled

$(eval $(meson-package))
