# Railway Signal Controller: CS254 Course Project

## Team Speckled Band 
* [160050098] Saiteja Talluri
* [160050104] Jagadeep Sai
* [160050069] Shantanu Kumar
* [160050095] Kalyan S.s

## Code of Conduct
"We pledge on our honour that we have not given or received any unauthorized assistance on this assignment or any previous task."

## Instructions
1. Clone the **Railway-Signal-Controller** repository and rename it as **Speckled_Band** folder.
1. Paste the **Speckled_Band** folder besides the **20140524** folder.
3. Cut the **script.sh** in the **Speckled_Band** folder and paste it besides the **20140524** folder and run these commands.
```
chmod 777 script.sh
bash script.sh
```

## Assumptions and Details
1. Convention used for 
  a. Displaying : Led 7 - red,Led 6 - amber, Led 5 - green, Led 4,3 - Nothing, Led 2,1,0- Direction
  b. Data by switch : Data 7 - track_exist, Data 6 - track_ok, Data 5,4,3 - Direction, Data 2,1,0 - Next signal
2. After pressing "UP", host expects data from "DOWN" within 20 seconds.
3. The controller spends 8 seconds in state S6.
4. After the display of leds, controller goes to S3 if "UP" is pressed else to S4 or S6.
5. When "UP" is pressed a value is sent to host from controller to indicate "UP" has been pressed.
6. Then the host reads for data till 20 secs and then waits 50 secs (given for the UART communication) after receiving 
   the data given from "DOWN" and then polling restarts.
7. If "UP" is not pressed, the host will wait till 60 sec (given for the UART communication) and then polling restarts.

## UART Communication

1. We have implemented both the mandatory and the optional UART part. 

2. The host folder has two c files uart1.c and uart2.c. Compile them using these commands 
```
gcc -o uart1 uart1.c
gcc -o uart2 uart2.c
```
3. Then run the following commands one on each terminal after connecting both the uart ports to the laptop.
(Ensure both ports are ready by **lsusb** or **ls /dev/ttyX***).
```
./uart1
./uart2 
```
4. You can see the data being transfered in binary and decimal in the terminals.
5. You can send data on uart from one board to another when the first board is in state S4 ,the second board 
   will receive the data anytime but update the uart data in the board when it next goes to state S5, it will 
   display the uart data (for 3 seconds) after displaying all the 8 directions when it reaches state S2 afer 
   having encountered S5 once.


## Citation
1. https://github.com/makestuff/libfpgalink for the base code and libraries.
2. For Uart communication, http://www.bealto.com/fpga-uart.html provided us with a basic_uart module which did the 
   uart read and write when run from cksum_rtl as portmap.
