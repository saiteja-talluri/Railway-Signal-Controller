/* 
 * Copyright (C) 2012-2014 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <makestuff.h>
#include <libfpgalink.h>
#include <libbuffer.h>
#include <liberror.h>

#include <libdump.h>
#include <argtable2.h>
#include <readline/readline.h>
#include <readline/history.h>
#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

bool sigIsRaised(void);
void sigRegisterHandler(void);
FILE* in;
static const char *ptr;
static bool enableBenchmarking = false;

int read_line[8];


/* power --> calculates the value of x raised to the power n */  

int power (int x,int n)
{
    int number = 1;

    for (int i = 0; i < n; ++i)
        number *= x;

    return number;
}

int str_len(char* s){
	int i = 0;
	for(;;i++){
		if(s[i] == '\0')
			break;
	}
	return i;
}

char* str_cat(char* s1,char* s2){
	int l1 = str_len(s1);
	int l2 = str_len(s2);
	char* s = (char*) malloc ((l1+l2+1)*sizeof(char));
	for(int i = 0;i < l1; i++)
		s[i] = s1[i];
	for(int i = 0;i < l2;i++)
		s[i+l1] = s2[i];
	s[l1+l2] = '\0';
	return s;
}

/* string_compare --> if two strings are equal then it returns true else false */

bool string_compare(char* s1,char* s2,int n){
	bool flag = false;
	for (int i = 0; i < n; i++){
		if(*(s1+i) == *(s2+i))
			flag=true;
		else{
			flag = false;
			break;
			}
		}

	return flag;
}

/* bin2hex --> given a binary string and number of bits in it, converts it into hexdecimal */

char* bin2hex(char* str,int n){
	int y = n/4;
	char* hex = (char*) malloc ((y+1)*sizeof(char));
	for(int i = 0 ;i < y; i++){
		char ch[5];
		ch[0] = str[i*4];
		ch[1] = str[i*4+1];
		ch[2] = str[i*4+2];
		ch[3] = str[i*4+3];
		ch[4] = '\0';

		if(!strcmp(ch,"0000"))
			hex[i] = '0';
		else if(!strcmp(ch,"0001"))
			hex[i] = '1';
		else if(!strcmp(ch,"0010"))
			hex[i] = '2';
		else if(!strcmp(ch,"0011"))
			hex[i] = '3';
		else if(!strcmp(ch,"0100"))
			hex[i] = '4';
		else if(!strcmp(ch,"0101"))
			hex[i] = '5';
		else if(!strcmp(ch,"0110"))
			hex[i] = '6';
		else if(!strcmp(ch,"0111"))
			hex[i] = '7';
		else if(!strcmp(ch,"1000"))
			hex[i] = '8';
		else if(!strcmp(ch,"1001"))
			hex[i] = '9';
		else if(!strcmp(ch,"1010"))
			hex[i] = 'a';
		else if(!strcmp(ch,"1011"))
			hex[i] = 'b';
		else if(!strcmp(ch,"1100"))
			hex[i] = 'c';
		else if(!strcmp(ch,"1101"))
			hex[i] = 'd';
		else if(!strcmp(ch,"1110"))
			hex[i] = 'e';
		else if(!strcmp(ch,"1111"))
			hex[i] = 'f';
	}
	hex[y] = '\0';
	return hex;
}

/* int2bin --> given a number converts it into 8 bit binary string */

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

/* bin2int --> given a binary string and its length converts it into decimal number */

int bin2int(char* s, int n){
	int a = 0;
	for(int i = n-1; i >= 0;i--){
		if(s[i] =='1'){
			a += power(2,n-i-1);
		}
	}
	
	return a;
}

/* append --> given a string, it appends it 8 times and gives the appended string */

char* append(char t[]){
	char* s = (char*) malloc (33*sizeof(char));
	for(int i = 0 ; i < 8; i++){
		s[i*4]   = t[0];
		s[i*4+1] = t[1];
		s[i*4+2] = t[2];
		s[i*4+3] = t[3];
	}
	s[32] = '\0';
	return s;
}

/* do_xor --> does bitwise xor of two given strings */

char* do_xor(char s1[],char s2[]){
	char* s = (char*) malloc (33*sizeof(char));
	for(int i = 0; i < str_len(s1); i++){
		if(s1[i] == s2[i])
			s[i] = '0';
		else
			s[i] = '1';
	}
	s[32] = '\0';
	return s;
}

/* int_bin_4 --> does int to binary conversion and gives 4 bit binary strings */

char* int_bin_4(int n){
	if(n >= 16)
		n = n-16;
	char* s = (char*) malloc (5*sizeof(char));
	for(int i = 3; i >= 0;i--){
		s[i] = n%2 + 48;
		n = n/2;
	}
	s[4] = '\0';
	return s;
}

/* encrypt  --> implements the encryption protocol given in the problem statement */

char* encrypt(char data[],char key[]){
	int n = 0;
	for(int i = 0;i < 32; i++){
		if(key[i] == '1')
			n++;
	}
	int a[4];
	char* cipher = (char*) malloc (33*sizeof(char));
	for(int i = 0; i < 32; i++)
		cipher[i] = data[i];
	cipher[32] = '\0';
	char* t = (char*) malloc (5*sizeof(char));
	for(int i = 0; i < 4; i++){
		a[i] = 0;
		for(int j = i; j < 32; j = j + 4){
			if(key[j] == '1')
				a[i] = a[i]^1;
		}
		if(a[i] == 0)
			t[i] = '0';
		else 
			t[i] = '1';
	}
	t[4] = '\0';
	for(int i = 0; i < n; i++){
		cipher = do_xor(cipher,append(t));
		t = int_bin_4(bin2int(t,4)+1);
	}
	return cipher;
}

/* decrypt  --> implements the decryption protocol given in the problem statement */

char* decrypt(char code[],char key[]){
	int n = 0;
	for(int i = 0; i<32; i++){
		if(key[i] == '0')
			n++;
	}
	int a[4];
	char* data = (char*) malloc (33*sizeof(char));
	
	for(int i=0;i<32;i++)
		data[i] = code[i];
	data[32] = '\0';
	char* t = (char*) malloc (5*sizeof(char));

	for(int i = 0; i < 4; i++){
		a[i] = 0;
		for(int j = i;j < 32;j = j + 4){
			if(key[j] == '1')
				a[i] = a[i]^1;
		}
		if(a[i] == 0)
			t[i] = '0';
		else 
			t[i] = '1';
	}
	t[4] = '\0';

	t = int_bin_4(bin2int(t,4) + 15);

	for(int i = 0; i < n; i++){
		data = do_xor(data,append(t));
		t = int_bin_4(bin2int(t,4) + 15);
	}

	return data;
}


char* output_ans(int x,int y){
	in = fopen("track_data.csv","r"); /* open file on command line */
	if(in == NULL){
		perror("File open error");
		exit(EXIT_FAILURE);
	} 
	char* ans = (char*) malloc (65*sizeof(char));
	for(int i = 0;i < 64; i++)
		ans[i] = '0';
	ans[64] = '\0';
	ans[12] = '1';ans[19] = '1';ans[27] = '1';
	ans[28] = '1';ans[34] = '1';ans[42] = '1';
	ans[44] = '1';ans[50] = '1';ans[51] = '1';
	ans[58] = '1';ans[59] = '1';ans[60] = '1';
	// ans="0000000000001000000100000001100000100000001010000011000000111000";
	int a,b,c,d,e;
	while(fscanf(in, "%d,%d,%d,%d,%d", &a,&b,&d,&c,&e)!= EOF){
		char next_sig[3]="000";
		if(e >= 4){
			next_sig[0] = '1';e -= 4;
		}
		if(e >= 2){
			next_sig[1] = '1';e -= 2;
		}
		if(e >= 1){
			next_sig[2] = '1';
		}
		
		if(a==x&&b==y){
			ans[d*8] = '1';
			if(c==1)
				ans[d*8+1] = '1';
			else
				ans[d*8+1] = '0';

			ans[d*8+5] = next_sig[0];
			ans[d*8+6] = next_sig[1];
			ans[d*8+7] = next_sig[2];
		}
	}
	fclose(in);

	return ans;
}

void table_update(int x,int y,int dir,int t_ok,int nxt_sig){
	FILE* in = fopen("track_data.csv","r");
    if(in == NULL){
        perror("File open error");
        exit(EXIT_FAILURE);
    }
    FILE* out = fopen("temp.csv","w");
    if(out == NULL){
        perror("File open error");
        exit(EXIT_FAILURE);
    }   
    int a,b,c,d,e;
    int check=0;
    while(fscanf(in, "%d,%d,%d,%d,%d", &a,&b,&c,&d,&e)!= EOF){
        if(a==x&&b==y&&c==dir){
        	fprintf(out,"%d,%d,%d,%d,%d\n",a,b,c,t_ok,nxt_sig);	
        	check=1;
        }
		else{
			fprintf(out,"%d,%d,%d,%d,%d\n",a,b,c,d,e);
		}
    }
    if(check==0)
    	fprintf(out,"%d,%d,%d,%d,%d\n",x,y,dir,t_ok,nxt_sig);

    fclose(in);
    fclose(out);
	if(system("sudo chmod 777 temp.csv")==-1){
		printf("system com not work\n");
	}
	if(system("mv temp.csv track_data.csv")==-1){
		printf("System command not executed\n");
	}
}



static bool isHexDigit(char ch) {
	return
		(ch >= '0' && ch <= '9') ||
		(ch >= 'a' && ch <= 'f') ||
		(ch >= 'A' && ch <= 'F');
}

static uint16 calcChecksum(const uint8 *data, size_t length) {
	uint16 cksum = 0x0000;
	while ( length-- ) {
		cksum = (uint16)(cksum + *data++);
	}
	return cksum;
}

static bool getHexNibble(char hexDigit, uint8 *nibble) {
	if ( hexDigit >= '0' && hexDigit <= '9' ) {
		*nibble = (uint8)(hexDigit - '0');
		return false;
	} else if ( hexDigit >= 'a' && hexDigit <= 'f' ) {
		*nibble = (uint8)(hexDigit - 'a' + 10);
		return false;
	} else if ( hexDigit >= 'A' && hexDigit <= 'F' ) {
		*nibble = (uint8)(hexDigit - 'A' + 10);
		return false;
	} else {
		return true;
	}
}

static int getHexByte(uint8 *byte) {
	uint8 upperNibble;
	uint8 lowerNibble;
	if ( !getHexNibble(ptr[0], &upperNibble) && !getHexNibble(ptr[1], &lowerNibble) ) {
		*byte = (uint8)((upperNibble << 4) | lowerNibble);
		byte += 2;
		return 0;
	} else {
		return 1;
	}
}

static const char *const errMessages[] = {
	NULL,
	NULL,
	"Unparseable hex number",
	"Channel out of range",
	"Conduit out of range",
	"Illegal character",
	"Unterminated string",
	"No memory",
	"Empty string",
	"Odd number of digits",
	"Cannot load file",
	"Cannot save file",
	"Bad arguments"
};

typedef enum {
	FLP_SUCCESS,
	FLP_LIBERR,
	FLP_BAD_HEX,
	FLP_CHAN_RANGE,
	FLP_CONDUIT_RANGE,
	FLP_ILL_CHAR,
	FLP_UNTERM_STRING,
	FLP_NO_MEMORY,
	FLP_EMPTY_STRING,
	FLP_ODD_DIGITS,
	FLP_CANNOT_LOAD,
	FLP_CANNOT_SAVE,
	FLP_ARGS
} ReturnCode;

static ReturnCode doRead(
	struct FLContext *handle, uint8 chan, uint32 length, FILE *destFile, uint16 *checksum,
	const char **error)
{	
	ReturnCode retVal = FLP_SUCCESS;
	uint32 bytesWritten;
	FLStatus fStatus;
	uint32 chunkSize;
	const uint8 *recvData;
	uint32 actualLength;
	const uint8 *ptr;
	uint16 csVal = 0x0000;
	#define READ_MAX 65536

	// Read first chunk
	chunkSize = length >= READ_MAX ? READ_MAX : length;
	fStatus = flReadChannelAsyncSubmit(handle, chan, chunkSize, NULL, error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");
	length = length - chunkSize;

	while ( length ) {
		// Read chunk N
		chunkSize = length >= READ_MAX ? READ_MAX : length;
		fStatus = flReadChannelAsyncSubmit(handle, chan, chunkSize, NULL, error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");
		length = length - chunkSize;
		
		// Await chunk N-1
		fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");

		// Write chunk N-1 to file
		bytesWritten = (uint32)fwrite(recvData, 1, actualLength, destFile);
		CHECK_STATUS(bytesWritten != actualLength, FLP_CANNOT_SAVE, cleanup, "doRead()");

		// Checksum chunk N-1
		chunkSize = actualLength;
		ptr = recvData;
		while ( chunkSize-- ) {
			csVal = (uint16)(csVal + *ptr++);
		}
	}

	// Await last chunk
	fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");
	
	// Write last chunk to file
	bytesWritten = (uint32)fwrite(recvData, 1, actualLength, destFile);
	CHECK_STATUS(bytesWritten != actualLength, FLP_CANNOT_SAVE, cleanup, "doRead()");

	// Checksum last chunk
	chunkSize = actualLength;
	ptr = recvData;
	while ( chunkSize-- ) {
		csVal = (uint16)(csVal + *ptr++);
	}
	
	// Return checksum to caller
	*checksum = csVal;
cleanup:
	return retVal;
}

static ReturnCode doWrite(
	struct FLContext *handle, uint8 chan, FILE *srcFile, size_t *length, uint16 *checksum,
	const char **error)
{
	ReturnCode retVal = FLP_SUCCESS;
	size_t bytesRead, i;
	FLStatus fStatus;
	const uint8 *ptr;
	uint16 csVal = 0x0000;
	size_t lenVal = 0;
	#define WRITE_MAX (65536 - 5)
	uint8 buffer[WRITE_MAX];

	do {
		// Read Nth chunk
		bytesRead = fread(buffer, 1, WRITE_MAX, srcFile);
		if ( bytesRead ) {
			// Update running total
			lenVal = lenVal + bytesRead;

			// Submit Nth chunk
			fStatus = flWriteChannelAsync(handle, chan, bytesRead, buffer, error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doWrite()");

			// Checksum Nth chunk
			i = bytesRead;
			ptr = buffer;
			printf("InBuffer: %s::\n",buffer);
			while ( i-- ) {
				csVal = (uint16)(csVal + *ptr++);
			}
		}
	} while ( bytesRead == WRITE_MAX );

	// Wait for writes to be received. This is optional, but it's only fair if we're benchmarking to
	// actually wait for the work to be completed.
	fStatus = flAwaitAsyncWrites(handle, error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doWrite()");

	// Return checksum & length to caller
	*checksum = csVal;
	*length = lenVal;
cleanup:
	return retVal;
}

static int parseLine(struct FLContext *handle, const char *line, const char **error) {
	ReturnCode retVal = FLP_SUCCESS, status;
	FLStatus fStatus;
	struct Buffer dataFromFPGA = {0,};
	BufferStatus bStatus;
	uint8 *data = NULL;
	char *fileName = NULL;
	FILE *file = NULL;
	double totalTime, speed;
	#ifdef WIN32
		LARGE_INTEGER tvStart, tvEnd, freq;
		DWORD_PTR mask = 1;
		SetThreadAffinityMask(GetCurrentThread(), mask);
		QueryPerformanceFrequency(&freq);
	#else
		struct timeval tvStart, tvEnd;
		long long startTime, endTime;
	#endif
	bStatus = bufInitialise(&dataFromFPGA, 1024, 0x00, error);
	CHECK_STATUS(bStatus, FLP_LIBERR, cleanup);
	ptr = line;
	do {
		while ( *ptr == ';' ) {
			ptr++;
		}
		switch ( *ptr ) {
		case 'r':{
			uint32 chan;
			uint32 length = 1;
			char *end;
			ptr++;
			
			// Get the channel to be read:
			errno = 0;
			chan = (uint32)strtoul(ptr, &end, 16);
			CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);

			// Ensure that it's 0-127
			CHECK_STATUS(chan > 127, FLP_CHAN_RANGE, cleanup);
			ptr = end;

			// Only three valid chars at this point:
			CHECK_STATUS(*ptr != '\0' && *ptr != ';' && *ptr != ' ', FLP_ILL_CHAR, cleanup);

			if ( *ptr == ' ' ) {
				ptr++;

				// Get the read count:
				errno = 0;
				length = (uint32)strtoul(ptr, &end, 16);
				
				CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);
				ptr = end;
				
				// Only three valid chars at this point:
				CHECK_STATUS(*ptr != '\0' && *ptr != ';' && *ptr != ' ', FLP_ILL_CHAR, cleanup);
				if ( *ptr == ' ' ) {
					const char *p;
					const char quoteChar = *++ptr;
					CHECK_STATUS(
						(quoteChar != '"' && quoteChar != '\''),
						FLP_ILL_CHAR, cleanup);
					
					// Get the file to write bytes to:
					ptr++;
					p = ptr;
					while ( *p != quoteChar && *p != '\0' ) {
						p++;
					}
					CHECK_STATUS(*p == '\0', FLP_UNTERM_STRING, cleanup);
					fileName = malloc((size_t)(p - ptr + 1));
					CHECK_STATUS(!fileName, FLP_NO_MEMORY, cleanup);
					CHECK_STATUS(p - ptr == 0, FLP_EMPTY_STRING, cleanup);
					strncpy(fileName, ptr, (size_t)(p - ptr));
					fileName[p - ptr] = '\0';
					ptr = p + 1;
				}
			}
			if ( fileName ) {
				uint16 checksum = 0x0000;

				// Open file for writing
				file = fopen(fileName, "wb");
				CHECK_STATUS(!file, FLP_CANNOT_SAVE, cleanup);
				free(fileName);
				fileName = NULL;

				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					status = doRead(handle, (uint8)chan, length, file, &checksum, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
					status = doRead(handle, (uint8)chan, length, file, &checksum, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Read %d bytes (checksum 0x%04X) from channel %d at %f MiB/s\n",
						length, checksum, chan, speed);
				}
				CHECK_STATUS(status, status, cleanup);

				// Close the file
				fclose(file);
				file = NULL;
			} else {
				size_t oldLength = dataFromFPGA.length;
				bStatus = bufAppendConst(&dataFromFPGA, 0x00, length, error);
				CHECK_STATUS(bStatus, FLP_LIBERR, cleanup);
				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					fStatus = flReadChannel(handle, (uint8)chan, length, dataFromFPGA.data + oldLength, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);

					printf("Length: %d",length);
					
					fStatus = flReadChannel(handle, (uint8)chan, length, dataFromFPGA.data + oldLength, error);
					
					for(size_t i = 0; i < length; i++){
						read_line[i] = *(dataFromFPGA.data + oldLength + i);
					}

					//Data Read

					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Read %d bytes (checksum 0x%04X) from channel %d at %f MiB/s\n",
						length, calcChecksum(dataFromFPGA.data + oldLength, length), chan, speed);
				}
				CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			}
			break;
		}
		case 'w':{
			unsigned long int chan;
			size_t length = 1, i;
			char *end, ch;
			const char *p;
			ptr++;
			
			// Get the channel to be written:
			errno = 0;
			chan = strtoul(ptr, &end, 16);
			CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);

			// Ensure that it's 0-127
			CHECK_STATUS(chan > 127, FLP_CHAN_RANGE, cleanup);
			ptr = end;

			// There must be a space now:
			CHECK_STATUS(*ptr != ' ', FLP_ILL_CHAR, cleanup);

			// Now either a quote or a hex digit
		   ch = *++ptr;
			if ( ch == '"' || ch == '\'' ) {
				uint16 checksum = 0x0000;

				// Get the file to read bytes from:
				ptr++;
				p = ptr;
				while ( *p != ch && *p != '\0' ) {
					p++;
				}
				CHECK_STATUS(*p == '\0', FLP_UNTERM_STRING, cleanup);
				fileName = malloc((size_t)(p - ptr + 1));
				CHECK_STATUS(!fileName, FLP_NO_MEMORY, cleanup);
				CHECK_STATUS(p - ptr == 0, FLP_EMPTY_STRING, cleanup);
				strncpy(fileName, ptr, (size_t)(p - ptr));
				fileName[p - ptr] = '\0';
				ptr = p + 1;  // skip over closing quote

				// Open file for reading
				file = fopen(fileName, "rb");
				CHECK_STATUS(!file, FLP_CANNOT_LOAD, cleanup);
				free(fileName);
				fileName = NULL;
				
				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					status = doWrite(handle, (uint8)chan, file, &length, &checksum, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
					status = doWrite(handle, (uint8)chan, file, &length, &checksum, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Wrote "PFSZD" bytes (checksum 0x%04X) to channel %lu at %f MiB/s\n",
						length, checksum, chan, speed);
				}
				CHECK_STATUS(status, status, cleanup);

				// Close the file
				fclose(file);
				file = NULL;
			} else if ( isHexDigit(ch) ) {
				// Read a sequence of hex bytes to write
				uint8 *dataPtr;
				p = ptr + 1;
				while ( isHexDigit(*p) ) {
					p++;
				}
				CHECK_STATUS((p - ptr) & 1, FLP_ODD_DIGITS, cleanup);
				length = (size_t)(p - ptr) / 2;
				data = malloc(length);
				dataPtr = data;
				for ( i = 0; i < length; i++ ) {
					getHexByte(dataPtr++);
					ptr += 2;
				}
				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					fStatus = flWriteChannel(handle, (uint8)chan, length, data, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
					
					
					fStatus = flWriteChannel(handle, (uint8)chan, length, data, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Wrote "PFSZD" bytes (checksum 0x%04X) to channel %lu at %f MiB/s\n",
						length, calcChecksum(data, length), chan, speed);
				}
				CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
				free(data);
				data = NULL;
			} else {
				FAIL(FLP_ILL_CHAR, cleanup);
			}
			break;
		}
		//----------------------------------------------------------------------------------------
		case 'y':{
			unsigned long int chan;
			size_t length = 1, i;
			char *end, ch;
			const char *p;
			ptr++;
			
			// Get the channel to be written:
			errno = 0;
			chan = strtoul(ptr, &end, 16);
			CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);

			// Ensure that it's 0-127
			CHECK_STATUS(chan > 127, FLP_CHAN_RANGE, cleanup);
			ptr = end;

			// There must be a space now:
			CHECK_STATUS(*ptr != ' ', FLP_ILL_CHAR, cleanup);

			// Now either a quote or a hex digit
		   ch = *++ptr;
			if ( ch == '"' || ch == '\'' ) {
				uint16 checksum = 0x0000;

				// Get the file to read bytes from:
				ptr++;
				p = ptr;
				while ( *p != ch && *p != '\0' ) {
					p++;
				}
				CHECK_STATUS(*p == '\0', FLP_UNTERM_STRING, cleanup);
				fileName = malloc((size_t)(p - ptr + 1));
				CHECK_STATUS(!fileName, FLP_NO_MEMORY, cleanup);
				CHECK_STATUS(p - ptr == 0, FLP_EMPTY_STRING, cleanup);
				strncpy(fileName, ptr, (size_t)(p - ptr));
				fileName[p - ptr] = '\0';
				ptr = p + 1;  // skip over closing quote

				// Open file for reading
				file = fopen(fileName, "rb");
				CHECK_STATUS(!file, FLP_CANNOT_LOAD, cleanup);
				free(fileName);
				fileName = NULL;
				
				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					status = doWrite(handle, (uint8)chan, file, &length, &checksum, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
					status = doWrite(handle, (uint8)chan, file, &length, &checksum, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Wrote "PFSZD" bytes (checksum 0x%04X) to channel %lu at %f MiB/s\n",
						length, checksum, chan, speed);
				}
				CHECK_STATUS(status, status, cleanup);

				// Close the file
				fclose(file);
				file = NULL;
			} else if ( isHexDigit(ch) ) {
				// Read a sequence of hex bytes to write
				uint8 *dataPtr;
				p = ptr + 1;
				while ( isHexDigit(*p) ) {
					p++;
				}
					

				CHECK_STATUS((p - ptr) & 1, FLP_ODD_DIGITS, cleanup);
				length = (size_t)(p - ptr) / 2;
				data = malloc(length);
				dataPtr = data;
				for ( i = 0; i < length; i++ ) {
					getHexByte(dataPtr++);
					ptr += 2;
				}
				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					fStatus = flWriteChannel(handle, (uint8)chan, length, data, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);

					
					//Data Write

					fStatus = flWriteChannel(handle, (uint8)chan, length, data, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Wrote "PFSZD" bytes (checksum 0x%04X) to channel %lu at %f MiB/s\n",
						length, calcChecksum(data, length), chan, speed);
				}
				CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
				free(data);
				data = NULL;
			} else {
				FAIL(FLP_ILL_CHAR, cleanup);
			}
			break;
		}
		// ---------------------------------------------------------------------
		case '+':{
			uint32 conduit;
			char *end;
			ptr++;

			// Get the conduit
			errno = 0;
			conduit = (uint32)strtoul(ptr, &end, 16);
			CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);

			// Ensure that it's 0-127
			CHECK_STATUS(conduit > 255, FLP_CONDUIT_RANGE, cleanup);
			ptr = end;

			// Only two valid chars at this point:
			CHECK_STATUS(*ptr != '\0' && *ptr != ';', FLP_ILL_CHAR, cleanup);

			fStatus = flSelectConduit(handle, (uint8)conduit, error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			break;
		}
		default:
			FAIL(FLP_ILL_CHAR, cleanup);
		}
	} while ( *ptr == ';' );
	CHECK_STATUS(*ptr != '\0', FLP_ILL_CHAR, cleanup);

	dump(0x00000000, dataFromFPGA.data, dataFromFPGA.length);

cleanup:
	bufDestroy(&dataFromFPGA);
	if ( file ) {
		fclose(file);
	}
	free(fileName);
	free(data);
	if ( retVal > FLP_LIBERR ) {
		const int column = (int)(ptr - line);
		int i;
		fprintf(stderr, "%s at column %d\n  %s\n  ", errMessages[retVal], column, line);
		for ( i = 0; i < column; i++ ) {
			fprintf(stderr, " ");
		}
		fprintf(stderr, "^\n");
	}
	return retVal;
}

static const char *nibbles[] = {
	"0000",  // '0'
	"0001",  // '1'
	"0010",  // '2'
	"0011",  // '3'
	"0100",  // '4'
	"0101",  // '5'
	"0110",  // '6'
	"0111",  // '7'
	"1000",  // '8'
	"1001",  // '9'

	"XXXX",  // ':'
	"XXXX",  // ';'
	"XXXX",  // '<'
	"XXXX",  // '='
	"XXXX",  // '>'
	"XXXX",  // '?'
	"XXXX",  // '@'

	"1010",  // 'A'
	"1011",  // 'B'
	"1100",  // 'C'
	"1101",  // 'D'
	"1110",  // 'E'
	"1111"   // 'F'
};

int main(int argc, char *argv[]) {
	ReturnCode retVal = FLP_SUCCESS, pStatus;
	struct arg_str *ivpOpt = arg_str0("i", "ivp", "<VID:PID>", "            vendor ID and product ID (e.g 04B4:8613)");
	struct arg_str *vpOpt = arg_str1("v", "vp", "<VID:PID[:DID]>", "       VID, PID and opt. dev ID (e.g 1D50:602B:0001)");
	struct arg_str *fwOpt = arg_str0("f", "fw", "<firmware.hex>", "        firmware to RAM-load (or use std fw)");
	struct arg_str *portOpt = arg_str0("d", "ports", "<bitCfg[,bitCfg]*>", " read/write digital ports (e.g B13+,C1-,B2?)");
	struct arg_str *queryOpt = arg_str0("q", "query", "<jtagBits>", "         query the JTAG chain");
	struct arg_str *progOpt = arg_str0("p", "program", "<config>", "         program a device");
	struct arg_uint *conOpt = arg_uint0("c", "conduit", "<conduit>", "        which comm conduit to choose (default 0x01)");
	struct arg_str *actOpt = arg_str0("a", "action", "<actionString>", "    a series of CommFPGA actions");
	struct arg_lit *shellOpt  = arg_lit0("s", "shell", "                    start up an interactive CommFPGA session");
	struct arg_lit *benOpt  = arg_lit0("b", "benchmark", "                enable benchmarking & checksumming");
	struct arg_lit *rstOpt  = arg_lit0("r", "reset", "                    reset the bulk endpoints");
	struct arg_str *dumpOpt = arg_str0("l", "dumploop", "<ch:file.bin>", "   write data from channel ch to file");
	struct arg_lit *helpOpt  = arg_lit0("h", "help", "                     print this help and exit");
	struct arg_str *eepromOpt  = arg_str0(NULL, "eeprom", "<std|fw.hex|fw.iic>", "   write firmware to FX2's EEPROM (!!)");
	struct arg_str *backupOpt  = arg_str0(NULL, "backup", "<kbitSize:fw.iic>", "     backup FX2's EEPROM (e.g 128:fw.iic)\n");
	struct arg_lit *myshellOpt  = arg_lit0("m", "myshell", "                 my   start up an interactive CommFPGA session");
	struct arg_end *endOpt   = arg_end(20);
	void *argTable[] = {
		ivpOpt, vpOpt, fwOpt, portOpt, queryOpt, progOpt, conOpt, actOpt,
		shellOpt, myshellOpt, benOpt, rstOpt, dumpOpt, helpOpt, eepromOpt, backupOpt, endOpt 
	};
	const char *progName = "flcli";
	int numErrors;
	struct FLContext *handle = NULL;
	FLStatus fStatus;
	const char *error = NULL;
	const char *ivp = NULL;
	const char *vp = NULL;
	bool isNeroCapable, isCommCapable;
	uint32 numDevices, scanChain[16], i;
	const char *line = NULL;
	uint8 conduit = 0x01;

	if ( arg_nullcheck(argTable) != 0 ) {
		fprintf(stderr, "%s: insufficient memory\n", progName);
		FAIL(1, cleanup);
	}

	numErrors = arg_parse(argc, argv, argTable);

	if ( helpOpt->count > 0 ) {
		printf("FPGALink Command-Line Interface Copyright (C) 2012-2014 Chris McClelland\n\nUsage: %s", progName);
		arg_print_syntax(stdout, argTable, "\n");
		printf("\nInteract with an FPGALink device.\n\n");
		arg_print_glossary(stdout, argTable,"  %-10s %s\n");
		FAIL(FLP_SUCCESS, cleanup);
	}

	if ( numErrors > 0 ) {
		arg_print_errors(stdout, endOpt, progName);
		fprintf(stderr, "Try '%s --help' for more information.\n", progName);
		FAIL(FLP_ARGS, cleanup);
	}

	fStatus = flInitialise(0, &error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);

	vp = vpOpt->sval[0];

	printf("Attempting to open connection to FPGALink device %s...\n", vp);
	fStatus = flOpen(vp, &handle, NULL);
	if ( fStatus ) {
		if ( ivpOpt->count ) {
			int count = 60;
			uint8 flag;
			ivp = ivpOpt->sval[0];
			printf("Loading firmware into %s...\n", ivp);
			if ( fwOpt->count ) {
				fStatus = flLoadCustomFirmware(ivp, fwOpt->sval[0], &error);
			} else {
				fStatus = flLoadStandardFirmware(ivp, vp, &error);
			}
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			
			printf("Awaiting renumeration");
			flSleep(1000);
			do {
				printf(".");
				fflush(stdout);
				fStatus = flIsDeviceAvailable(vp, &flag, &error);
				CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
				flSleep(250);
				count--;
			} while ( !flag && count );
			printf("\n");
			if ( !flag ) {
				fprintf(stderr, "FPGALink device did not renumerate properly as %s\n", vp);
				FAIL(FLP_LIBERR, cleanup);
			}

			printf("Attempting to open connection to FPGLink device %s again...\n", vp);
			fStatus = flOpen(vp, &handle, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		} else {
			fprintf(stderr, "Could not open FPGALink device at %s and no initial VID:PID was supplied\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	printf(
		"Connected to FPGALink device %s (firmwareID: 0x%04X, firmwareVersion: 0x%08X)\n",
		vp, flGetFirmwareID(handle), flGetFirmwareVersion(handle)
	);

	if ( eepromOpt->count ) {
		if ( !strcmp("std", eepromOpt->sval[0]) ) {
			printf("Writing the standard FPGALink firmware to the FX2's EEPROM...\n");
			fStatus = flFlashStandardFirmware(handle, vp, &error);
		} else {
			printf("Writing custom FPGALink firmware from %s to the FX2's EEPROM...\n", eepromOpt->sval[0]);
			fStatus = flFlashCustomFirmware(handle, eepromOpt->sval[0], &error);
		}
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
	}

	if ( backupOpt->count ) {
		const char *fileName;
		const uint32 kbitSize = strtoul(backupOpt->sval[0], (char**)&fileName, 0);
		if ( *fileName != ':' ) {
			fprintf(stderr, "%s: invalid argument to option --backup=<kbitSize:fw.iic>\n", progName);
			FAIL(FLP_ARGS, cleanup);
		}
		fileName++;
		printf("Saving a backup of %d kbit from the FX2's EEPROM to %s...\n", kbitSize, fileName);
		fStatus = flSaveFirmware(handle, kbitSize, fileName, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
	}

	if ( rstOpt->count ) {
		// Reset the bulk endpoints (only needed in some virtualised environments)
		fStatus = flResetToggle(handle, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
	}

	if ( conOpt->count ) {
		conduit = (uint8)conOpt->ival[0];
	}

	isNeroCapable = flIsNeroCapable(handle);
	isCommCapable = flIsCommCapable(handle, conduit);

	if ( portOpt->count ) {
		uint32 readState;
		char hex[9];
		const uint8 *p = (const uint8 *)hex;
		printf("Configuring ports...\n");
		fStatus = flMultiBitPortAccess(handle, portOpt->sval[0], &readState, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		sprintf(hex, "%08X", readState);
		printf("Readback:   28   24   20   16    12    8    4    0\n          %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf("  %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s\n", nibbles[*p++ - '0']);
		flSleep(100);
	}

	if ( queryOpt->count ) {
		if ( isNeroCapable ) {
			fStatus = flSelectConduit(handle, 0x00, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = jtagScanChain(handle, queryOpt->sval[0], &numDevices, scanChain, 16, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			if ( numDevices ) {
				printf("The FPGALink device at %s scanned its JTAG chain, yielding:\n", vp);
				for ( i = 0; i < numDevices; i++ ) {
					printf("  0x%08X\n", scanChain[i]);
				}
			} else {
				printf("The FPGALink device at %s scanned its JTAG chain but did not find any attached devices\n", vp);
			}
		} else {
			fprintf(stderr, "JTAG chain scan requested but FPGALink device at %s does not support NeroProg\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( progOpt->count ) {
		printf("Programming device...\n");
		if ( isNeroCapable ) {
			fStatus = flSelectConduit(handle, 0x00, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flProgram(handle, progOpt->sval[0], NULL, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		} else {
			fprintf(stderr, "Program operation requested but device at %s does not support NeroProg\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( benOpt->count ) {
		enableBenchmarking = true;
	}
	
	if ( actOpt->count ) {
		printf("Executing CommFPGA actions on FPGALink device %s...\n", vp);
		if ( isCommCapable ) {
			uint8 isRunning;
			fStatus = flSelectConduit(handle, conduit, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flIsFPGARunning(handle, &isRunning, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			if ( isRunning ) {
				pStatus = parseLine(handle, actOpt->sval[0], &error);
				CHECK_STATUS(pStatus, pStatus, cleanup);
			} else {
				fprintf(stderr, "The FPGALink device at %s is not ready to talk - did you forget --program?\n", vp);
				FAIL(FLP_ARGS, cleanup);
			}
		} else {
			fprintf(stderr, "Action requested but device at %s does not support CommFPGA\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( dumpOpt->count ) {
		const char *fileName;
		unsigned long chan = strtoul(dumpOpt->sval[0], (char**)&fileName, 10);
		FILE *file = NULL;
		const uint8 *recvData;
		uint32 actualLength;
		if ( *fileName != ':' ) {
			fprintf(stderr, "%s: invalid argument to option -l|--dumploop=<ch:file.bin>\n", progName);
			FAIL(FLP_ARGS, cleanup);
		}
		fileName++;
		printf("Copying from channel %lu to %s", chan, fileName);
		file = fopen(fileName, "wb");
		CHECK_STATUS(!file, FLP_CANNOT_SAVE, cleanup);
		sigRegisterHandler();
		fStatus = flSelectConduit(handle, conduit, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		fStatus = flReadChannelAsyncSubmit(handle, (uint8)chan, 22528, NULL, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		do {
			fStatus = flReadChannelAsyncSubmit(handle, (uint8)chan, 22528, NULL, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fwrite(recvData, 1, actualLength, file);
			printf(".");
		} while ( !sigIsRaised() );
		printf("\nCaught SIGINT, quitting...\n");
		fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		fwrite(recvData, 1, actualLength, file);
		fclose(file);
	}

	if ( shellOpt->count ) {
		printf("\nEntering CommFPGA command-line mode:\n");
		if ( isCommCapable ) {
		    uint8 isRunning;
			fStatus = flSelectConduit(handle, conduit, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flIsFPGARunning(handle, &isRunning, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			if ( isRunning ) {
				
				do {
					do {
						line = readline("> ");
					} while ( line && !line[0] );
					if ( line && line[0] && line[0] != 'q' ) {

						add_history(line);
						pStatus = parseLine(handle, line, &error);
						CHECK_STATUS(pStatus, pStatus, cleanup);
						free((void*)line);
					}
				} while ( line && line[0] != 'q' );
			} else {
				fprintf(stderr, "The FPGALink device at %s is not ready to talk - did you forget --xsvf?\n", vp);
				FAIL(FLP_ARGS, cleanup);
			}
		} else {
			fprintf(stderr, "Shell requested but device at %s does not support CommFPGA\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( myshellOpt->count ) {
		printf("\nEntering My Shell CommFPGA command-line mode:\n");

		/* Key instantiation */
		char K[] = "11010100101010111001001011101010";
		printf("before while \n");
		if ( isCommCapable ) {
		    uint8 isRunning;
			fStatus = flSelectConduit(handle, conduit, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flIsFPGARunning(handle, &isRunning, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);

			int R1_1,R1_2,R1_3,R1_4;

			/* ACK1 and ACK2 instantiation */
			char * ACK1 = "11111111000000001111111100000000"; 
			char * ACK2 = "00000000111111110000000011111111"; 
			if ( isRunning ) {
				char* Readch;
				char* Writech;
				char* Dec_Coordinate;
				char* Enc_Coordinate;
				// char* BR1_1,BR1_2,BR1_3,BR1_4,BR;
				char* ACK1_received;
				char* Data_Update;
				char* Enc_ACK2;
				char* Hex_ACK2;
				char* ans;
				char ans1[33];
				char ans2[33];
				char* enc_ans1;
				char* enc_ans2;
				char* final_ans_f8;
				char* final_ans_l8;
				char* Hex_Enc_Coordinate;
				bool correct_ack1;
				int read_channel;
				int write_channel;
				char* read_ch;
				char* write_ch;
				char* Hex_Enc_final_ans_f8;
				char* Hex_Enc_final_ans_l8;
				while(true){
					int i;
					for(i = 0;i < 64;i++){
						printf("in for loop %d \n",i);
						read_channel = 2*i;
						write_channel = 2*i+1;
						read_ch = int2bin(read_channel);
						read_ch = bin2hex(read_ch,8);
						write_ch = int2bin(write_channel);
						write_ch = bin2hex(write_ch,8);
						Readch = str_cat("r",read_ch);
						Readch = str_cat(Readch," 4");
						Writech = str_cat("w",write_ch);
						Writech = str_cat(Writech," ");
						printf("Receiving the encrypted coordinates from the FPGA Board %s \n",Readch);
						pStatus = parseLine(handle, Readch, &error);
						CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_1 = read_line[0];
						// pStatus = parseLine(handle, Readch, &error);
						// CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_2 = read_line[1];
						// pStatus = parseLine(handle, Readch, &error);
						// CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_3 = read_line[2];
						// pStatus = parseLine(handle, Readch, &error);
						// CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_4 = read_line[3];
						// printf("%d\n",R1_4 );
						char* BR1_1 = int2bin(R1_1);
						char* BR1_2 = int2bin(R1_2);
						char* BR1_3 = int2bin(R1_3);
						char* BR1_4 = int2bin(R1_4);					
						// printf("before br\n");
						char* BR = BR1_1;
						// printf("after br\n");
						BR = str_cat(BR,BR1_2);
						// printf("final br\n");
						BR = str_cat(BR,BR1_3);
						BR = str_cat(BR,BR1_4);
						printf("Encrypted coordinates in binary %s\n", BR);
						printf("Decrypting the encrypted coordinates \n");
						Dec_Coordinate = decrypt(BR,K);
						printf("actual coordinates  %s\n", Dec_Coordinate);
						// Re-encrypting the coordinates and sending back
						Enc_Coordinate=encrypt(Dec_Coordinate,K);
						printf("Encrypted coordinates %s\n",Enc_Coordinate );
						printf("Writing the coordinates on channel 0 \n");
						// Writing the coordinates on channel 0
						Enc_Coordinate = bin2hex(Enc_Coordinate,32);

						Hex_Enc_Coordinate = str_cat(Writech,Enc_Coordinate);

						printf("%s\n",Hex_Enc_Coordinate );
						pStatus = parseLine(handle, Hex_Enc_Coordinate, &error);
						CHECK_STATUS(pStatus, pStatus, cleanup);
						printf("receiving encrypted ACK1  \n");
						//receiving encrypted ACK1 
						pStatus = parseLine(handle, Readch, &error);
						CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_1 = read_line[0];
						// pStatus = parseLine(handle, Readch, &error);
						// CHECK_STATUS(pStatus, pStatus, cleanup);
					  R1_2 = read_line[1];
						// pStatus = parseLine(handle, Readch, &error);
						// CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_3 = read_line[2];
						// pStatus = parseLine(handle, Readch, &error);
						// CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_4 = read_line[3];

						BR1_1 = int2bin(R1_1);
						BR1_2 = int2bin(R1_2);
						BR1_3 = int2bin(R1_3);
						BR1_4 = int2bin(R1_4);
						BR = BR1_1;
						BR = str_cat(BR,BR1_2);
						BR = str_cat(BR,BR1_3);
						BR = str_cat(BR,BR1_4);

						ACK1_received = decrypt(BR,K);

						printf("checking ACK1  \n");
						//checking ACK1
						correct_ack1 = string_compare(ACK1_received,ACK1,32);
						if(correct_ack1){
							break;
						}
						else{
							sleep(5);
							pStatus = parseLine(handle, Readch, &error);
							CHECK_STATUS(pStatus, pStatus, cleanup);
							R1_1 = read_line[0];
							// pStatus = parseLine(handle, Readch, &error);
							// CHECK_STATUS(pStatus, pStatus, cleanup);
							R1_2 = read_line[1];
							// pStatus = parseLine(handle, Readch, &error);
							// CHECK_STATUS(pStatus, pStatus, cleanup);
							R1_3 = read_line[2];
							// pStatus = parseLine(handle, Readch, &error);
							// CHECK_STATUS(pStatus, pStatus, cleanup);
							R1_4 = read_line[3];

							BR1_1 = int2bin(R1_1);
							BR1_2 = int2bin(R1_2);
							BR1_3 = int2bin(R1_3);
							BR1_4 = int2bin(R1_4);
							BR = BR1_1;
							BR = str_cat(BR,BR1_2);
							BR = str_cat(BR,BR1_3);
							BR = str_cat(BR,BR1_4);

							ACK1_received = decrypt(BR,K);
							correct_ack1 = string_compare(ACK1_received,ACK1,32);
							if(correct_ack1){
								break;
							}
							else{
								if(i == 63)
									i=0;
							}
						}
					}

					printf("sending ACK2 \n");
					///sending ACK2
					Enc_ACK2 = encrypt(ACK2,K);
					Enc_ACK2 = bin2hex(Enc_ACK2,32);
					Hex_ACK2 = str_cat(Writech,Enc_ACK2);
					printf("ACK2 is %s\n",ACK2 );
					printf(" ACK2 in hex is %s\n",Hex_ACK2 );
					pStatus = parseLine(handle, Hex_ACK2, &error);
					CHECK_STATUS(pStatus, pStatus, cleanup);

					char X[5];
					X[0] = Dec_Coordinate[0];
					X[1] = Dec_Coordinate[1];
					X[2] = Dec_Coordinate[2];
					X[3] = Dec_Coordinate[3];
					X[4] = '\0';

					char Y[5];
					Y[0] = Dec_Coordinate[4];
					Y[1] = Dec_Coordinate[5];
					Y[2] = Dec_Coordinate[6];
					Y[3] = Dec_Coordinate[7];
					Y[4] = '\0';

					printf("Searching the input in table\n");
					int X_int = bin2int(X,4);
					int Y_int = bin2int(Y,4);
					// printf("before output \n");
					ans = output_ans(X_int,Y_int);
					// printf("after output  \n");
					printf("%s\n",ans);

					
					for(int k = 0;k < 32; k++)
						ans1[k] = ans[k];
					ans1[32] = '\0';

					for(int k = 0;k < 32; k++)
						ans2[k] = ans[k+32];
					ans2[32] = '\0';

					enc_ans1 = encrypt(ans1,K);
					enc_ans2 = encrypt(ans2,K);
					enc_ans1[32] = '\0';
					enc_ans2[32] = '\0';
					printf("encrypted ans1 %s\n",enc_ans1);
					printf("encrypted ans2 %s\n",enc_ans2);
					
					final_ans_f8=bin2hex(enc_ans1,32);
					final_ans_l8=bin2hex(enc_ans2,32);
					
					printf("writing the 1st 4 bytes\n");

					Hex_Enc_final_ans_f8=str_cat(Writech,final_ans_f8);
					
					printf("%s\n", Hex_Enc_final_ans_f8);
					pStatus = parseLine(handle, Hex_Enc_final_ans_f8, &error);
					CHECK_STATUS(pStatus, pStatus, cleanup);
					int counter = 0;
					int ack_receipt = 0;
					while(counter < 33){
						//recieving ACK1
						pStatus = parseLine(handle, Readch, &error);
						CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_1 = read_line[0];
						// pStatus = parseLine(handle, Readch, &error);
						// CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_2 = read_line[1];
						// pStatus = parseLine(handle, Readch, &error);
						// CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_3 = read_line[2];
						// pStatus = parseLine(handle, Readch, &error);
						// CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_4 = read_line[3];

						char* BR1_1 = int2bin(R1_1);
						char* BR1_2 = int2bin(R1_2);
						char* BR1_3 = int2bin(R1_3);
						char* BR1_4 = int2bin(R1_4);
						char* BR = BR1_1;
						BR = str_cat(BR,BR1_2);
						BR = str_cat(BR,BR1_3);
						BR = str_cat(BR,BR1_4);

						ACK1_received = decrypt(BR,K);
						printf("ack1 received\n");
						//checking ACK1
						correct_ack1 = string_compare(ACK1_received,ACK1,32);
						if(correct_ack1){
							ack_receipt = 1;
							break;
						}
						counter++;
						sleep(8);
					}
					if(ack_receipt == 0)
						continue;
					printf("writing the 2nd 4 bytes\n");

					Hex_Enc_final_ans_l8 = str_cat(Writech,final_ans_l8);
					
					pStatus = parseLine(handle, Hex_Enc_final_ans_l8, &error);
					CHECK_STATUS(pStatus, pStatus, cleanup);
					printf("%s\n",Hex_Enc_final_ans_l8 );
					printf("2nd 4 bytes written\n");

					counter = 0;
					ack_receipt = 0;
					while(counter < 33){
						//recieving ACK1
						pStatus = parseLine(handle, Readch, &error);
						CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_1 = read_line[0];
						// pStatus = parseLine(handle, Readch, &error);
						// CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_2 = read_line[1];
						// pStatus = parseLine(handle, Readch, &error);
						// CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_3 = read_line[2];
						// pStatus = parseLine(handle, Readch, &error);
						// CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_4 = read_line[3];

						char* BR1_1 = int2bin(R1_1);
						char* BR1_2 = int2bin(R1_2);
						char* BR1_3 = int2bin(R1_3);
						char* BR1_4 = int2bin(R1_4);
						char* BR = BR1_1;
						BR = str_cat(BR,BR1_2);
						BR = str_cat(BR,BR1_3);
						BR = str_cat(BR,BR1_4);

						ACK1_received = decrypt(BR,K);
						printf("ack1 received\n");
						//checking ACK1
						correct_ack1 = string_compare(ACK1_received,ACK1,32);
						if(correct_ack1){
							ack_receipt = 1;
							break;
						}
						counter++;
						sleep(8);
					}
					if(ack_receipt == 0)
						continue;

					Enc_ACK2 = encrypt(ACK2,K);
					Enc_ACK2 = bin2hex(Enc_ACK2,32);
					Hex_ACK2 = str_cat(Writech,Enc_ACK2);					
					pStatus = parseLine(handle, Hex_ACK2, &error);
					CHECK_STATUS(pStatus, pStatus, cleanup);
					printf("ack2 sent\n");
						
					sleep(28);

					pStatus = parseLine(handle, Readch, &error);
					CHECK_STATUS(pStatus, pStatus, cleanup);
					R1_1 = read_line[0];
					// pStatus = parseLine(handle, Readch, &error);
					// CHECK_STATUS(pStatus, pStatus, cleanup);
					R1_2 = read_line[1];
					// pStatus = parseLine(handle, Readch, &error);
					// CHECK_STATUS(pStatus, pStatus, cleanup);
					R1_3 = read_line[2];
					// pStatus = parseLine(handle, Readch, &error);
					// CHECK_STATUS(pStatus, pStatus, cleanup);
					R1_4 = read_line[3];

					char* BR1_1 = int2bin(R1_1);
					char* BR1_2 = int2bin(R1_2);
					char* BR1_3 = int2bin(R1_3);
					char* BR1_4 = int2bin(R1_4);
					char* BR = BR1_1;
					BR = str_cat(BR,BR1_2);
					BR = str_cat(BR,BR1_3);
					BR = str_cat(BR,BR1_4);

					ACK1_received = decrypt(BR,K);
					
					//checking ACK1
					correct_ack1 = string_compare(ACK1_received,"11111111000000000000000000000000",32);
					if(!correct_ack1){
						sleep(60);
						continue;
					}
					printf("Up Pressed\n");
					int S3_time=0;
					while(S3_time<20){
						S3_time++;
						printf("in while loop %d\n",S3_time);
						pStatus = parseLine(handle, Readch, &error);
						CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_1 = read_line[0];
						// pStatus = parseLine(handle, Readch, &error);
						// CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_2 = read_line[1];
						// pStatus = parseLine(handle, Readch, &error);
						// CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_3 = read_line[2];
						// pStatus = parseLine(handle, Readch, &error);
						// CHECK_STATUS(pStatus, pStatus, cleanup);
						R1_4 = read_line[3];

						char* BR1_1 = int2bin(R1_1);
						char* BR1_2 = int2bin(R1_2);
						char* BR1_3 = int2bin(R1_3);
						char* BR1_4 = int2bin(R1_4);
						char* BR = BR1_1;
						BR = str_cat(BR,BR1_2);
						BR = str_cat(BR,BR1_3);
						BR = str_cat(BR,BR1_4);
						if(string_compare(BR,"00000000000000000000000000000000",32)){
							sleep(1);
							continue;
						}
						Data_Update = decrypt(BR,K);

						printf("data to be updated%s\n",Data_Update);

						char Direction[4];
						Direction[0]=Data_Update[2];
						Direction[1]=Data_Update[3];
						Direction[2]=Data_Update[4];
						Direction[3]='\0';

						char next_signal[4];
						next_signal[0]=Data_Update[5];
						next_signal[1]=Data_Update[6];
						next_signal[2]=Data_Update[7];
						next_signal[3]='\0';

						int nxt_sig=bin2int(next_signal,3);
						int dir=bin2int(Direction,3);
						int track_ok;

						if(Data_Update[0]=='1'){
							if(Data_Update[1]=='0')
								track_ok=0;
							else
								track_ok=1;

							table_update(X_int,Y_int,dir,track_ok,nxt_sig);
						}					
						sleep(50);
						break;
					}

						
				}
					
			} else {
				fprintf(stderr, "The FPGALink device at %s is not ready to talk - did you forget --xsvf?\n", vp);
				FAIL(FLP_ARGS, cleanup);
			}
		} else {
			fprintf(stderr, "Shell requested but device at %s does not support CommFPGA\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

cleanup:
	free((void*)line);
	flClose(handle);
	if ( error ) {
		fprintf(stderr, "%s\n", error);
		flFreeError(error);
	}
	return retVal;
}
