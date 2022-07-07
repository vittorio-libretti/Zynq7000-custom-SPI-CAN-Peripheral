
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "rs232.h"



int main(){
	
	
	int cport_nr = 3;        // /dev/ttyS'xx' (COM'xx+1' on windows)
	int bdrate = 115200;

	int n;
	unsigned char buf[4096];

	char mode[]={'8','N','1',0};

	
	if(RS232_OpenComport(cport_nr, bdrate, mode, 0)){
		printf("Cannot open comport\n");
		return(0);
	}

	char QUIT = 0;
	
	while(!QUIT){
		
		//RS232_cputs(cport_nr, str[i]);
		
		buf[0] = 'I'; buf[1] = 'D'; buf[2] = 0; buf[3] = 0; buf[4] = 0; buf[5] = 0; 
		RS232_SendBuf(cport_nr, buf, 6);
		printf("\nsent: ID0000");

		Sleep(600);

		n = RS232_PollComport(cport_nr, buf, 4095);
		/*if(n > 0){
			buf[n] = 0;
			for(int i=0; i < n; i++){
				if(buf[i] < 32){
					buf[i] = '.';  // replace unreadable control-codes by dots
				}
			}
			printf("received %d bytes: %s\n", n, (char *)buf);		
		}*/
		
		printf("received %d bytes: %s\n", n, (char *)buf);		
		
		Sleep(600);
		
		/*buf[0] = 'H'; buf[1] = 'M'; buf[2] = 'B'; buf[3] = 0; buf[4] = 0; buf[5] = 0; 
		RS232_SendBuf(cport_nr, buf, 6);
		printf("\nsent: HMB000");

		Sleep(600);

		n = RS232_PollComport(cport_nr, buf, 4095);
		if(n > 0){
			buf[n] = 0;
			for(int i=0; i < n; i++){
				if(buf[i] < 32){
					buf[i] = '.';  // replace unreadable control-codes by dots
				}
			}
			printf("received %d bytes: %s\n", n, (char *)buf);		
		}
		
		Sleep(600);*/
		
		
		QUIT = 1;
	}

	return(0);
}

