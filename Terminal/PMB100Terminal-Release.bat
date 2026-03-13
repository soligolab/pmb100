@echo off
REM Run this file to build the project outside of the IDE.
REM WARNING: if using a different machine, copy the .rsp files together with this script.
echo can_hw.cpp
d:\linux\SysGCC\beaglebone\bin\arm-linux-gnueabihf-g++.exe @"VisualGDB/Release/CanHW.gcc.rsp" || exit 1
echo can_master.cpp
d:\linux\SysGCC\beaglebone\bin\arm-linux-gnueabihf-g++.exe @"VisualGDB/Release/can_master.gcc.rsp" || exit 1
echo fram_spi.cpp
d:\linux\SysGCC\beaglebone\bin\arm-linux-gnueabihf-g++.exe @"VisualGDB/Release/FramSpi.gcc.rsp" || exit 1
echo led_net.cpp
d:\linux\SysGCC\beaglebone\bin\arm-linux-gnueabihf-g++.exe @"VisualGDB/Release/LedNet.gcc.rsp" || exit 1
echo led_seriali.cpp
d:\linux\SysGCC\beaglebone\bin\arm-linux-gnueabihf-g++.exe @"VisualGDB/Release/LedSeriali.gcc.rsp" || exit 1
echo pmb100_terminal.cpp
d:\linux\SysGCC\beaglebone\bin\arm-linux-gnueabihf-g++.exe @"VisualGDB/Release/PMB100Terminal.gcc.rsp" || exit 1
echo plc.cpp
d:\linux\SysGCC\beaglebone\bin\arm-linux-gnueabihf-g++.exe @"VisualGDB/Release/PLC.gcc.rsp" || exit 1
echo servizio_device_finder.cpp
d:\linux\SysGCC\beaglebone\bin\arm-linux-gnueabihf-g++.exe @"VisualGDB/Release/ServizioDeviceFinder.gcc.rsp" || exit 1
echo terminale_led.cpp
d:\linux\SysGCC\beaglebone\bin\arm-linux-gnueabihf-g++.exe @"VisualGDB/Release/TerminaleLed.gcc.rsp" || exit 1
echo timer.cpp
d:\linux\SysGCC\beaglebone\bin\arm-linux-gnueabihf-g++.exe @"VisualGDB/Release/timer.gcc.rsp" || exit 1
echo util.cpp
d:\linux\SysGCC\beaglebone\bin\arm-linux-gnueabihf-g++.exe @"VisualGDB/Release/Util.gcc.rsp" || exit 1
echo var.cpp
d:\linux\SysGCC\beaglebone\bin\arm-linux-gnueabihf-g++.exe @"VisualGDB/Release/var.gcc.rsp" || exit 1
echo Linking VisualGDB/Release/PMB100Terminal...
d:\linux\SysGCC\beaglebone\bin\arm-linux-gnueabihf-g++.exe @VisualGDB/Release/PMB100Terminal.link.rsp || exit 1
