ifneq ($(KERNELRELEASE),)

obj-m += buzzer.o

else

KDIR ?= /lib/modules/$(shell uname -r)/build

modules:
	$(MAKE) -C $(KDIR) M=$(CURDIR) $@

%:
	$(MAKE) -C $(KDIR) M=$(CURDIR) $@

endif
