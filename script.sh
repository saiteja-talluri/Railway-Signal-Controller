#!/bin/bash

CWD="$(pwd)"

cp $CWD/Speckled_Band/fpga/cksum_rtl.vhdl $CWD/20140524/makestuff/hdlmake/apps/makestuff/swled/cksum/vhdl/cksum_rtl.vhdl
cp $CWD/Speckled_Band/fpga/basic_uart.vhdl $CWD/20140524/makestuff/hdlmake/apps/makestuff/swled/cksum/vhdl/basic_uart.vhdl
cp $CWD/Speckled_Band/fpga/decrypter.vhdl $CWD/20140524/makestuff/hdlmake/apps/makestuff/swled/cksum/vhdl/decrypter.vhdl
cp $CWD/Speckled_Band/fpga/encrypter.vhdl $CWD/20140524/makestuff/hdlmake/apps/makestuff/swled/cksum/vhdl/encrypter.vhdl
cp $CWD/Speckled_Band/fpga/hdlmakecksum.cfg $CWD/20140524/makestuff/hdlmake/apps/makestuff/swled/cksum/vhdl/hdlmake.cfg
cp $CWD/Speckled_Band/fpga/running_all.sh $CWD/20140524/makestuff/hdlmake/apps/makestuff/swled/cksum/vhdl/running_all.sh
cp $CWD/Speckled_Band/fpga/track_data.csv $CWD/20140524/makestuff/hdlmake/apps/makestuff/swled/cksum/vhdl/track_data.csv

cp $CWD/Speckled_Band/fpga/harness.vhdl $CWD/20140524/makestuff/hdlmake/apps/makestuff/swled/templates/harness.vhdl
cp $CWD/Speckled_Band/fpga/board.ucf $CWD/20140524/makestuff/hdlmake/apps/makestuff/swled/templates/fx2all/boards/atlys/board.ucf

cp $CWD/Speckled_Band/fpga/debouncer.vhdl $CWD/20140524/makestuff/hdlmake/apps/makestuff/swled/templates/fx2all/vhdl/debouncer.vhdl
cp $CWD/Speckled_Band/fpga/top_level.vhdl $CWD/20140524/makestuff/hdlmake/apps/makestuff/swled/templates/fx2all/vhdl/top_level.vhdl
cp $CWD/Speckled_Band/fpga/hdlmaketop.cfg $CWD/20140524/makestuff/hdlmake/apps/makestuff/swled/templates/fx2all/vhdl/hdlmake.cfg

cp $CWD/Speckled_Band/host/main.c $CWD/20140524/makestuff/apps/flcli/main.c
cp $CWD/Speckled_Band/host/libfpgalink.c $CWD/20140524/makestuff/libs/libfpgalink/libfpgalink.c
cp $CWD/Speckled_Band/host/libusbwrap.c $CWD/20140524/makestuff/libs/libusbwrap/libusbwrap.c

cd $CWD/20140524/makestuff/libs/libfpgalink/
make

cd $CWD/20140524/makestuff/libs/libusbwrap/
make

cd $CWD/20140524/makestuff/apps/flcli/
make deps

cd $CWD/20140524/makestuff/hdlmake/apps/makestuff/swled/cksum/vhdl/
chmod 777 running_all.sh
./running_all.sh
# export PATH=$PATH:/opt/Xilinx/14.7/ISE_DS/ISE/bin/lin64
# ../../../../../bin/hdlmake.py -t ../../templates/fx2all/vhdl -b atlys -p fpga;
# sudo ../../../../../../apps/flcli/lin.x64/rel/flcli -v 1d50:602b:0002 -i 1443:0007;
# sudo ../../../../../../apps/flcli/lin.x64/rel/flcli -v 1d50:602b:0002 -p J:D0D2D3D4:fpga.xsvf;

#sudo gtkterm -p /dev/ttyXRUSB0 -s 115200 &
#cd $CWD/20140524/makestuff/hdlmake/apps/makestuff/swled/cksum/vhdl/
#sudo ../../../../../../apps/flcli/lin.x64/rel/flcli -v 1d50:602b:0002 -m