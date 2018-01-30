PKG_NAME=gecko_sdk
PKG_URL=https://github.com/kaibeckmann/RIOT-gecko-sdk
PKG_VERSION=d502a594fe05c1cb1ca0d96639c37b14a219f3f8
PKG_LICENSE=Zlib

ifneq ($(CPU),efm32)
  $(error This package can only be used with EFM32 CPUs)
endif

.PHONY: all

all: git-download
	"$(MAKE)" -C $(PKG_BUILDDIR)/dist

include $(RIOTBASE)/pkg/pkg.mk
