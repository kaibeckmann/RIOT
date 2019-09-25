FLASHER = $(RIOTTOOLS)/jlink/jlink.sh
DEBUGGER = $(RIOTTOOLS)/jlink/jlink.sh
DEBUGSERVER = $(RIOTTOOLS)/jlink/jlink.sh
RESET ?= $(RIOTTOOLS)/jlink/jlink.sh

# if IP for debug adapter is set, use VCOM over telnet
ifneq (,$(JLINK_IP))
  TERMPROG = $(RIOTTOOLS)/jlink/jlink.sh
  TERMFLAGS = term_vcom
endif

FLASHFILE ?= $(BINFILE)

FFLAGS ?= flash $(FLASHFILE)
DEBUGGER_FLAGS ?= debug $(ELFFILE)
DEBUGSERVER_FLAGS ?= debug-server
RESET_FLAGS ?= reset
