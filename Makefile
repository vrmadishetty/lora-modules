#
# Helpers to build modules out of linux-next based LoRa patch queue
#

KDIR ?= /lib/modules/`uname -r`/build

SDIR ?= $$PWD/linux
IDIR = $$PWD/include

MFLAGS_KCONFIG := CONFIG_LORA=m
MFLAGS_KCONFIG += CONFIG_LORA_DEV=m
MFLAGS_KCONFIG += CONFIG_LORA_RAK811=m
MFLAGS_KCONFIG += CONFIG_LORA_RN2483=m
MFLAGS_KCONFIG += CONFIG_LORA_SX1257=m
MFLAGS_KCONFIG += CONFIG_LORA_SX1276=m
MFLAGS_KCONFIG += CONFIG_LORA_SX1301=m
MFLAGS_KCONFIG += CONFIG_LORA_TING01M=m
MFLAGS_KCONFIG += CONFIG_LORA_USI=m
MFLAGS_KCONFIG += CONFIG_LORA_WIMOD=m

all: test
#	$(MAKE) -C $(KDIR) M=$$PWD
	$(MAKE) -C $(KDIR) M=$(SDIR)/net/lora $(MFLAGS_KCONFIG) \
		CFLAGS_MODULE=-I$(IDIR)
	$(MAKE) -C $(KDIR) M=$(SDIR)/drivers/net/lora \
		$(MFLAGS_KCONFIG) CFLAGS_MODULE=-I$(IDIR)

modules_install:
	for m in $(SDIR)/net/lora $(SDIR)/drivers/net/lora; do \
		$(MAKE) -C $(KDIR) M=$$m $(MFLAGS_KCONFIG) modules_install; \
	done

clean:
	$(MAKE) -C $(KDIR) M=$(SDIR)/net/lora $(MFLAGS_KCONFIG) clean
	$(MAKE) -C $(KDIR) M=$(SDIR)/drivers/net/lora $(MFLAGS_KCONFIG) clean
	@rm -f test

test: test.c
	$(CC) -o test test.c
