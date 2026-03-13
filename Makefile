.PHONY: all clean am3358 am3358-k49 am3358-k6 debug

all: am3358-k6

am3358:
	$(MAKE) -C Terminal all

# Kernel 4.9: usa GPIO sysfs (senza libgpiod)
am3358-k49:
	$(MAKE) -C Terminal all KERNEL_API=4.9

# Kernel 6.x: usa libgpiod (character device GPIO)
am3358-k6:
	$(MAKE) -C Terminal all KERNEL_API=6

debug:
	$(MAKE) -C Terminal all BUILD=debug KERNEL_API=6

clean:
	$(MAKE) -C Terminal clean
