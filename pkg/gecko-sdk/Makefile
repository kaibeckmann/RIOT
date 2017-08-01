PKG_NAME    =gecko-sdk
PKG_VERSION =v1.1
					   # name of the package
PKG_URL     =http://devtools.silabs.com/studio/v4/updates/binary/# source url of the package e.g. a git repository
PKG_DL_NAME =com.silabs.sdk.gecko_platform.v1.1.feature_root_1.1.1.201707211032-80# version of the package to use e.g. a git commit/ref
PKG_EXT     =zip# extension of this package

ifneq ($(RIOTBASE),)
include $(RIOTBASE)/Makefile.base
endif


# todo
# patch (from EFM2RIOT)
# for sdk/Device/SiliconLabs/{family}/Include/*.h
#  - define for __cpluspus and extern C ?
#  - fix ARM_MATH_CM0PLUS with define guard

.PHONY: all clean patch distclean

all: patch
#	$(MAKE) -C $(CURDIR)/$(PKG_NAME)-$(PKG_VERSION)

patch: $(CURDIR)/sdk/
	# Dependancy might be changed accordingly though we think the Makefile
	# will be the first thing you want to change
	#
	# Here might not happen anything besides dependancy checks

$(CURDIR)/sdk/: $(CURDIR)/.extracted 
	# Create sdk folder
	mkdir -p $(CURDIR)/sdk/
	# Copy sdk content to sdk folder
	mv $(CURDIR)/.extracted/developer/sdks/gecko_sdk_suite/$(PKG_VERSION)/* $(CURDIR)/sdk/
	# remove not needed stuff
	rm -rf $(CURDIR)/sdk/util
	rm -rf $(CURDIR)/sdk/meta
	rm -rf $(CURDIR)/sdk/platform/Device/SiliconLabs/em3*
	rm -rf $(CURDIR)/sdk/platform/Device/SiliconLabs/sky66107
	rm -rf $(CURDIR)/.extracted/* 
	# Here you apply your patch.
#		cd $< && patch ../patch.txt

#$(CURDIR)/$(PKG_NAME)-$(PKG_VERSION)/: $(CURDIR)/$(PKG_NAME)-$(PKG_VERSION).$(PKG_EXT)
	# Here you unpack the file.
	# This example assumes the common pattern that the archive contains its data in a subfolder with the same name as itself.
#	$(Q)$(UNZIP_HERE) $<


$(CURDIR)/.extracted: $(CURDIR)/$(PKG_NAME)-$(PKG_VERSION).$(PKG_EXT)
	rm -rf $@
	mkdir -p $@
	$(Q)cd $@ && $(UNZIP_HERE) $(CURDIR)/$(PKG_NAME)-$(PKG_VERSION).$(PKG_EXT)

$(CURDIR)/$(PKG_NAME)-$(PKG_VERSION).$(PKG_EXT):
	# Get PKG_VERSION of package from PKG_URL
	$(Q)$(DOWNLOAD_TO_FILE) $@ $(PKG_URL)/$(PKG_DL_NAME)

clean::
	# Reset package to checkout state.
	rm -rf $(CURDIR)/$(PKG_NAME)-$(PKG_VERSION) $(CURDIR)/sdk $(CURDIR)/.extracted
#	&& $(MAKE) $(CURDIR)/$(PKG_NAME)-$(PKG_VERSION)/Makefile

distclean::
	rm -rf $(CURDIR)/$(PKG_NAME)-$(PKG_VERSION) $(CURDIR)/$(PKG_NAME)-$(PKG_VERSION).$(PKG_EXT) $(CURDIR)/sdk $(CURDIR)/.extracted
