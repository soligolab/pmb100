# PMB100 Terminal build (ARMHF cross-compile)
TOOLCHAIN_ROOT ?= $(HOME)/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf
CROSS_COMPILE ?= $(TOOLCHAIN_ROOT)/bin/arm-linux-gnueabihf-

CXX := $(CROSS_COMPILE)g++
CC  := $(CROSS_COMPILE)gcc
STRIP := $(CROSS_COMPILE)strip

TARGET := terminal.bin

SRCS := \
	CanHW.cpp \
	can_master.cpp \
	FramSpi.cpp \
	LedNet.cpp \
	LedSeriali.cpp \
	PMB100Terminal.cpp \
	PLC.cpp \
	TerminaleLed.cpp \
	timer.cpp \
	Util.cpp \
	var.cpp

OBJS := $(SRCS:.cpp=.o)

BUILD ?= release
USE_GPIOD ?= 1

COMMON_DEFS := NOUPS=1 NOFINDER=1
ifeq ($(BUILD),debug)
	CXXFLAGS += -O0 -g -DDEBUG=1
else
	CXXFLAGS += -O2 -DNDEBUG=1 -DRELEASE=1
endif

CXXFLAGS += -std=gnu++17 -Wall -Wextra -I. $(addprefix -D,$(COMMON_DEFS))
LDFLAGS += -pthread

ifeq ($(USE_GPIOD),1)
	CXXFLAGS += -DPMB100_ENABLE_GPIOD=1
	LDLIBS += -lgpiod
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
