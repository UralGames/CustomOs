################################################################################
#
# swaylock
#
################################################################################

SWAYLOCK_VERSION = 1.8.0
SWAYLOCK_SITE = $(call github,swaywm,swaylock,v$(SWAYLOCK_VERSION))
SWAYLOCK_LICENSE = MIT
SWAYLOCK_LICENSE_FILES = LICENSE
SWAYLOCK_DEPENDENCIES = host-pkgconf host-wayland cairo gdk-pixbuf libxkbcommon \
	linux-pam wayland wayland-protocols
SWAYLOCK_CONF_OPTS = \
	-Dpam=enabled \
	-Dgdk-pixbuf=enabled \
	-Dman-pages=disabled \
	-Dzsh-completions=false \
	-Dbash-completions=false \
	-Dfish-completions=false

# Use the Meridian lock PAM policy (plain pam_unix password check).
define SWAYLOCK_INSTALL_PAM_POLICY
	$(INSTALL) -D -m 0644 $(BR2_EXTERNAL_CUSTOMOS_PATH)/../desktop/login/pam/meridian-lock \
		$(TARGET_DIR)/etc/pam.d/swaylock
endef
SWAYLOCK_POST_INSTALL_TARGET_HOOKS += SWAYLOCK_INSTALL_PAM_POLICY

$(eval $(meson-package))
