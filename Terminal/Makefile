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
	LDLIBS += -lgpiod
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
