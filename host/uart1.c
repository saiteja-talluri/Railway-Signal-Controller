#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static char* int2bin(int num){

	char* bini = (char*) malloc (9*sizeof(char));
	
	for(int i = 0; i < 8; i++)
		bini[i] = '0';
	
	bini[8] = '\0';

	int remainder;
    int j = 7;
    while (num > 0)
    {
        remainder = num % 2;
        bini[j] = '0' + remainder;
        num = num / 2;
        j--;
    }

    return bini;

}


int fd_new;
int fd;
int bytes_read;
void uart_init(int i)
{

/*------------------------------- Opening the Serial Port -------------------------------*/

FILE *in1 = NULL;
FILE *in2 = NULL;
char port1[15];
char port2[15];
in1 = popen("ls /dev/ttyX* | tail -1", "r");
fgets(port1,15, in1);
port1[14] = '\0';
in2 = popen("ls /dev/ttyX* | head -1", "r");
fgets(port2,15, in2);
port2[14] = '\0';
pclose(in1);
pclose(in2);
if(i == 0){
  fd_new = open(port1,O_RDWR | O_NOCTTY| O_SYNC);      /* !!blocks the read  */
                                           /* O_RDWR Read/Write access to serial port           */
                                                            /* O_NOCTTY - No terminal will control the process   */
                                                            /* O_NDELAY -Non Blocking Mode,Does not care about-  */
                                                            /* -the status of DCD line,Open() returns immediatly */
                                                            /* -the status of DCD line,Open() returns immediatly */
 if(fd_new == -1)                                               /* Error Checking */
  printf("\nError! in Opening %s  ",port1);
 else
  printf("\n%s Opened Successfully ",port1);

 /*---------- Setting the Attributes of the serial port using termios structure --------- */
  struct termios SerialPortSettings;          /* Create the structure                          */

  tcgetattr(fd_new, &SerialPortSettings);         /* Get the current attributes of the Serial port */

  cfsetispeed(&SerialPortSettings,B115200);        /* Set Read  Speed as 19200                       */
  cfsetospeed(&SerialPortSettings,B115200);        /* Set Write Speed as 19200                       */

  SerialPortSettings.c_cflag &= ~PARENB;          /* Disables the Parity   Enable bit(PARENB),So No Parity   */
  SerialPortSettings.c_cflag &= ~CSTOPB;          /* CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit */
  SerialPortSettings.c_cflag &= ~CSIZE;           /* Clears the mask for setting the data size             */
  SerialPortSettings.c_cflag |=  CS8;             /* Set the data bits = 8                                 */

  SerialPortSettings.c_cflag &= ~CRTSCTS;         /* No Hardware flow Control                         */
  SerialPortSettings.c_cflag |= CREAD | CLOCAL;   /* Enable receiver,Ignore Modem Control lines       */ 


  SerialPortSettings.c_iflag &= ~(IXON | IXOFF | IXANY);          /* Disable XON/XOFF flow control both i/p and o/p */
  SerialPortSettings.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                        | INLCR | IGNCR | ICRNL | IXON);
  SerialPortSettings.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);  
  SerialPortSettings.c_oflag &= ~OPOST;/*No Output Processing*/

  /* Setting Time outs */
  SerialPortSettings.c_cc[VMIN] = 1; /* Read at least 2 characters */
  SerialPortSettings.c_cc[VTIME] = 0; /* Wait indefinetly   */

  if((tcsetattr(fd_new,TCSANOW,&SerialPortSettings) != 0)) /* Set the attributes to the termios structure*/
  printf("\nERROR ! in Setting attributes");
  else
  printf("\nBaudRate = 115200\nStopBits = 1\nParity   = none\n");
}
else{
  fd = open(port2,O_RDWR | O_NOCTTY| O_SYNC);      /* !!blocks the read  */
                                           /* O_RDWR Read/Write access to serial port           */
                                                            /* O_NOCTTY - No terminal will control the process   */
                                                            /* O_NDELAY -Non Blocking Mode,Does not care about-  */
                                                            /* -the status of DCD line,Open() returns immediatly */
 if(fd == -1)                                               /* Error Checking */
  printf("\nError! in Opening %s  ",port2);
 else
  printf("\n%s Opened Successfully ",port2);

  /*---------- Setting the Attributes of the serial port using termios structure --------- */

  struct termios SerialPortSettings;          /* Create the structure                          */

  tcgetattr(fd, &SerialPortSettings);         /* Get the current attributes of the Serial port */

  cfsetispeed(&SerialPortSettings,B115200);        /* Set Read  Speed as 19200                       */
  cfsetospeed(&SerialPortSettings,B115200);        /* Set Write Speed as 19200                       */

  SerialPortSettings.c_cflag &= ~PARENB;          /* Disables the Parity   Enable bit(PARENB),So No Parity   */
  SerialPortSettings.c_cflag &= ~CSTOPB;          /* CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit */
  SerialPortSettings.c_cflag &= ~CSIZE;           /* Clears the mask for setting the data size             */
  SerialPortSettings.c_cflag |=  CS8;             /* Set the data bits = 8                                 */

  SerialPortSettings.c_cflag &= ~CRTSCTS;         /* No Hardware flow Control                         */
  SerialPortSettings.c_cflag |= CREAD | CLOCAL;   /* Enable receiver,Ignore Modem Control lines       */ 


  SerialPortSettings.c_iflag &= ~(IXON | IXOFF | IXANY);          /* Disable XON/XOFF flow control both i/p and o/p */
  SerialPortSettings.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                        | INLCR | IGNCR | ICRNL | IXON);
  SerialPortSettings.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);  
  SerialPortSettings.c_oflag &= ~OPOST;/*No Output Processing*/

  /* Setting Time outs */
  SerialPortSettings.c_cc[VMIN] = 1; /* Read at least 2 characters */
  SerialPortSettings.c_cc[VTIME] = 0; /* Wait indefinetly   */

  if((tcsetattr(fd_new,TCSANOW,&SerialPortSettings) != 0)) /* Set the attributes to the termios structure*/
  printf("\nERROR ! in Setting attributes");
  else
  printf("\nBaudRate = 115200\nStopBits = 1\nParity   = none\n");

}


}

char uart_receive(int id)
{
 char read_buffer[32];   /* Buffer to store the data received              */
 bytes_read = 0;    /* Number of bytes read by the read() system call */
 int i = 0;

 while(1){
   bytes_read = read(id,&read_buffer,1); /* Read the data                   */
   if(bytes_read>0)
    break;
 }

 printf("\n\nBytes Read %d", bytes_read); /* Print the number of bytes read */
 printf("\n\n");

 char ch = read_buffer[0];
 unsigned int y = ch;
 y = ((-1)*y) - 1;
 y = 255 - y;
 char *val = int2bin(y);
 printf("decimal: %d \n",y);
 printf("binary: %s \n",val); 

 if(bytes_read == 0){
  read_buffer[0] = '\0';
 }
 return read_buffer[0];
}

char uart_write(int id,char c){
    char write_buffer[32]; 
    write_buffer[0] = c;
    write_buffer[1] = '\0';
    int bytes_written = 0;
    bytes_written = write(id,write_buffer,1);
    printf("\n\nBytes Written %d", bytes_written); /* Print the number of bytes read */
    printf("\n\n");

    char ch = write_buffer[0];
    int y = ch;
    y = ((-1)*y) - 1;
    y = 255 - y;
    char *val = int2bin(y);
    printf("decimal: %d \n",y);
    printf("binary: %s \n",val); 

   return write_buffer[0];
}

void main(void)
{ 
  uart_init(0);
  uart_init(1);
  int iter = 1;
  while(1){
    printf("\nListening and Writing No :%d ",iter);
    char c  = uart_receive(fd);
    uart_write(fd_new,c);
    iter++;
  }
  close(fd_new);
  close(fd);
}