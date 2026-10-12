################################################################################
#
# libsfdo
#
################################################################################

LIBSFDO_VERSION = 0.1.4
LIBSFDO_SITE = https://gitlab.freedesktop.org/vyivel/libsfdo/-/archive/v$(LIBSFDO_VERSION)
LIBSFDO_SOURCE = libsfdo-v$(LIBSFDO_VERSION).tar.gz
LIBSFDO_LICENSE = BSD-2-Clause
LIBSFDO_LICENSE_FILES = LICENSE
LIBSFDO_INSTALL_STAGING = YES
LIBSFDO_DEPENDENCIES = host-pkgconf
LIBSFDO_CONF_OPTS = -Dexamples=false -Dtests=false

$(eval $(meson-package))
