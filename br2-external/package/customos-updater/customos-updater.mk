################################################################################
#
# customos-updater
#
################################################################################

CUSTOMOS_UPDATER_VERSION = $(shell cat $(BR2_EXTERNAL_CUSTOMOS_PATH)/../VERSION)
CUSTOMOS_UPDATER_SITE = $(BR2_EXTERNAL_CUSTOMOS_PATH)/../updater
CUSTOMOS_UPDATER_SITE_METHOD = local
CUSTOMOS_UPDATER_LICENSE = MIT
CUSTOMOS_UPDATER_LICENSE_FILES = LICENSE
CUSTOMOS_UPDATER_SUPPORTS_IN_SOURCE_BUILD = NO
CUSTOMOS_UPDATER_CMAKE_BACKEND = ninja
CUSTOMOS_UPDATER_DEPENDENCIES = host-qt6base qt6base rauc

CUSTOMOS_UPDATER_KEYRING = $(call qstrip,$(BR2_PACKAGE_CUSTOMOS_UPDATER_KEYRING))

CUSTOMOS_UPDATER_CONF_OPTS = \
	-DQT_HOST_PATH=$(HOST_DIR) \
	-DCUSTOMOS_UPDATE_REPOSITORY="$(call qstrip,$(BR2_PACKAGE_CUSTOMOS_UPDATER_REPOSITORY))" \
	-DCUSTOMOS_UPDATE_COMPATIBLE="$(call qstrip,$(BR2_PACKAGE_CUSTOMOS_UPDATER_COMPATIBLE))" \
	-DCUSTOMOS_UPDATE_CHANNEL="$(call qstrip,$(BR2_PACKAGE_CUSTOMOS_UPDATER_CHANNEL))"

# The image only ever contains the public CA certificate.
define CUSTOMOS_UPDATER_INSTALL_KEYRING
	@if [ ! -s "$(CUSTOMOS_UPDATER_KEYRING)" ]; then \
		echo "customos-updater: the trusted update certificate $(CUSTOMOS_UPDATER_KEYRING) is missing."; \
		echo "Create the signing keys with release/keys/create-ota-keys.sh (see docs/SIGNING.md)"; \
		echo "or point BR2_PACKAGE_CUSTOMOS_UPDATER_KEYRING at your CA certificate."; \
		exit 1; \
	fi
	@if grep -q "PRIVATE KEY" "$(CUSTOMOS_UPDATER_KEYRING)"; then \
		echo "customos-updater: $(CUSTOMOS_UPDATER_KEYRING) contains a private key; refusing to put it into the image."; \
		exit 1; \
	fi
	$(INSTALL) -D -m 0644 $(CUSTOMOS_UPDATER_KEYRING) $(TARGET_DIR)/etc/rauc/keyring.pem
endef

# Mount points of the A/B layout, per-slot kernel arguments, services.
define CUSTOMOS_UPDATER_INSTALL_SYSTEM
	mkdir -p $(TARGET_DIR)/data $(TARGET_DIR)/efi
	$(INSTALL) -D -m 0644 $(CUSTOMOS_UPDATER_PKGDIR)/cmdline.cfg \
		$(TARGET_DIR)/boot/customos/cmdline.cfg
	$(INSTALL) -D -m 0644 $(CUSTOMOS_UPDATER_PKGDIR)/60-customos-updater.preset \
		$(TARGET_DIR)/usr/lib/systemd/system-preset/60-customos-updater.preset
	$(INSTALL) -D -m 0644 $(CUSTOMOS_UPDATER_PKGDIR)/customos-watchdog.conf \
		$(TARGET_DIR)/usr/lib/systemd/system.conf.d/60-customos-watchdog.conf
	# /etc/rauc/system.conf -> /run/rauc/system.conf, written at boot for
	# the disk the system started from (installed by customos-updater).
	test -L $(TARGET_DIR)/etc/rauc/system.conf
endef

CUSTOMOS_UPDATER_POST_INSTALL_TARGET_HOOKS += \
	CUSTOMOS_UPDATER_INSTALL_KEYRING \
	CUSTOMOS_UPDATER_INSTALL_SYSTEM

$(eval $(cmake-package))
