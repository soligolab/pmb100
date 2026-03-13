# PMB100 Terminal build (AM3358 ARMHF cross-compile)
TOOLCHAIN_ROOT ?= $(HOME)/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf
CROSS_COMPILE ?= $(TOOLCHAIN_ROOT)/bin/arm-linux-gnueabihf-

CXX := $(CROSS_COMPILE)g++
CC  := $(CROSS_COMPILE)gcc
STRIP := $(CROSS_COMPILE)strip

TARGET := terminal.bin

SRCS := \
	can_hw.cpp \
	can_master.cpp \
	fram_spi.cpp \
	led_net.cpp \
	led_seriali.cpp \
	pmb100_terminal.cpp \
	plc.cpp \
	terminale_led.cpp \
	timer.cpp \
	util.cpp \
	var.cpp

OBJS := $(SRCS:.cpp=.o)

BUILD ?= release
# Valori supportati: 4.9, 6
KERNEL_API ?= 6

COMMON_DEFS := NOUPS=1 NOFINDER=1
ARCH_FLAGS := -march=armv7-a -mfpu=neon -mfloat-abi=hard -mtune=cortex-a8

ifeq ($(BUILD),debug)
	CXXFLAGS += -O0 -g -DDEBUG=1
else
	CXXFLAGS += -O2 -DNDEBUG=1 -DRELEASE=1
endif

CXXFLAGS += -std=gnu++17 -Wall -Wextra -I. $(ARCH_FLAGS) $(addprefix -D,$(COMMON_DEFS))
LDFLAGS += -pthread

ifeq ($(KERNEL_API),6)
CXXFLAGS += -DPMB100_ENABLE_GPIOD=1
GPIOD_LIB_FOUND := $(shell sh -c 'sysroot="$$( $(CXX) -print-sysroot 2>/dev/null )"; \
for p in "$$sysroot/usr/lib/libgpiod.so" "$$sysroot/usr/lib/libgpiod.a" "$$sysroot/usr/lib/arm-linux-gnueabihf/libgpiod.so" "$$sysroot/usr/lib/arm-linux-gnueabihf/libgpiod.a" "/usr/lib/libgpiod.so" "/usr/lib/libgpiod.a" "/usr/lib/arm-linux-gnueabihf/libgpiod.so" "/usr/lib/arm-linux-gnueabihf/libgpiod.a"; do [ -e "$$p" ] && { echo yes; exit 0; }; done; echo no')
ifeq ($(GPIOD_LIB_FOUND),yes)
LDLIBS += -lgpiod
else
$(warning libgpiod non trovata: build con fallback sysfs GPIO)
endif
endif

.PHONY: all clean strip

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

strip: $(TARGET)
	$(STRIP) $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
