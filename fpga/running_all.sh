#!/bin/bash

export PATH=$PATH:/opt/Xilinx/14.7/ISE_DS/ISE/bin/lin64
../../../../../bin/hdlmake.py -t ../../templates/fx2all/vhdl -b atlys -p fpga;
sudo ../../../../../../apps/flcli/lin.x64/rel/flcli -v 1d50:602b:0002 -i 1443:0007;
sudo ../../../../../../apps/flcli/lin.x64/rel/flcli -v 1d50:602b:0002 -p J:D0D2D3D4:fpga.xsvf;

