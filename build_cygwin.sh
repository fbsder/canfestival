#!/bin/bash
#export PCAN_INCLUDE=~/USB/Api/
#export PCAN_HEADER=Pcan_usb.h
#export PCAN_LIB=~/USB/Lib/Visual\ C++/Pcan_usb.lib 


export PCAN_INCLUDE=~/PC-Card/Api/
export PCAN_HEADER=Pcan_pcc.h
export PCAN_LIB=~/PC-Card/Lib/Visual\ C++/Pcan_pcc.lib 
export PCAN2_HEADER=Pcan_2pcc.h
export PCAN2_LIB=~/PC-Card/Lib/Visual\ C++/Pcan_2pcc.lib 
#./configure --can=peak_win32 --disable-dll
./configure --can=peak_win32
make clean all

 