.PHONY: all install

all: mkinitrd

mkinitrd: mkinitrd/mkinitrd
	$(MAKE) -C mkinitrd

install:
	$(MAKE) -C mkinitrd install

clean:
	$(MAKE) -C mkinitrd clean
