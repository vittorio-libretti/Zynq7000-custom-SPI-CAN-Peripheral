#include "SerialLink.h"

int SerialConnect(const char *path){

	int serial_port = -1;
	
	printf("Wait for serial device....\n");
	while(serial_port < 0){ serial_port = open (path,O_RDWR); }

	// Check for errors
	if(serial_port < 0){
		printf("Error %i from open: %s\n", errno, strerror(errno));	
	}
	else{
		printf("Serial opened succesfully!\n");
		//Create new termios struct, we call it 'tty' for convention
		//No need for "={0}" at the end as we'll immediately write the existing config to this struct
		struct termios tty;

		//Read in existing settings, and handle any error
		//NOTE: this is important! POSIX states that the struct passed to tcsetattr()
		//must have been initialized with a call to tcgetattr(), otherwise behaviour is undefined

		if(tcgetattr(serial_port, &tty) != 0){
			printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));		
		}
		else{
			printf("Device descriptor initialized correctly!\n");	
			/*termios struct reference extracted from termbits.h 

			struct termios {
				tcflag_t c_iflag;		//input mode flags
				tcflag_t c_oflag;		//output mode flags
				tcflag_t c_cflag;		//control mode flags 
				tcflag_t c_lflag;		//local mode flags 
				cc_t c_line;			//line discipline 
				cc_t c_cc[NCCS];		//control characters 
			};
			*/

			//c_cflag contains control parameter fields. 

			// PARENB: if this bit is set, generation and detection of the parity bit is enabed. Most serial communications
			// do not use a parity bit, so if you are unsure, clear this bit.
			tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
			//tty.c_cflag |= PARENB;  // Set parity bit, enabling parity

			// CSTOPB (Num stop bits): if this bit is set, two stop bits are used. if this is cleared, only one stop bit is used.
			// Most serial communications only use one stop bit.

			tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)

			//Number of bits per Byte: The CS<number> fields set how many data bits are transmitted per byte across the serial
			// port. the most common setting here is 8 (CS8). Definitely use this if you are unsure. Usually most serial ports
			// use 8 bits. You must clear all of the size bits before setting any of them with &= ~CSIZE

			tty.c_cflag &= ~CSIZE; // Clear all the size bits, then use one of the statements below
			tty.c_cflag |= CS8;

			/* Flow Control (CRTSCTS): If the CRTSCTS field is set, hardware RTS/CTS flow control is enabled. The most common 				setting here is to disable it. Enabling this when it should be disabled can result in your serial port receiving no 				data, as the sender will buffer it indefinitely, waiting for you to be “ready”.
			*/

			tty.c_cflag &= ~CRTSCTS;
			/*
			CREAD and CLOCAL: Setting CLOCAL disables modem-specific signal lines such as carrier detect. It also prevents the 				controlling process from getting sent a SIGHUP signal when a modem disconnect is detected, which is usually a good 				thing here. Setting CLOCAL allows us to read data (we definitely want that!).
			*/

			tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

			//LOCAL MODE FLAGS:
	
			/*Canonical mode: In canonical mode input is processed when a new line character is received. We do not
			want that when interacting with a serial port. Moreover in canonical mode some characters are special and usuallu
			ignored (like \). We do not want that in serial communication.*/

			tty.c_lflag &= ~ICANON;

			/*Echo: if the echo bit is set, sent characters will be echoed back. let's disable them*/
			tty.c_lflag &= ~ECHO;
			tty.c_lflag &= ~ECHOE; //DISABLE ERASURE
			tty.c_lflag &= ~ECHONL; //DISABLE NEW LINE ECHO

			/*Disable signal chars: when ISIG bit is set, INTR, QUIT and SUSP characters are interpreted. We don't want this with
			serial ports.*/

			tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP

			//INPUT MODES FLAGS: this flags contain low-level settings for input processing.

			/*Software flow control: clearing the IXOFF, IXON and IXANY disables software flow control*/
			
			tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
			
			/*Disabling special handling of Bytes On Receive: clearing all of the following  bits disables any
			special handling of the bytes as they are recieved by the serial port, before they are passed to the app. We
			just wants raw data!*/

			tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

			//OUTPUT MODES FLAG: the c_oflag member of termios contains low-level settings for output processing. When
			//dealing with a serial port, we want to disable any special handlung of output char/bytes

			tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
			tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

			//VMIN and VTIME (c_cc)
			/*When VMIN is 0, VTIME specifies a time-out from the start of the read() call. But when VMIN is > 0, VTIME specifies 				the time-out from the start of the first received character.

			VMIN = 0, VTIME = 0: No blocking, return immediately with what is available

			VMIN > 0, VTIME = 0: This will make read() always wait for bytes (exactly how many is determined by VMIN), so read() 				could block indefinitely.

			VMIN = 0, VTIME > 0: This is a blocking read of any number of chars with a maximum timeout (given by VTIME). read() 				will block until either any amount of data is available, or the timeout occurs. This happens to be my favourite mode 				(and the one I use the most).

			VMIN > 0, VTIME > 0: Block until either VMIN characters have been received, or VTIME after first character has 				elapsed. Note that the timeout for VTIME does not begin until the first character is received.

			*/

			/*VMIN and VTIME are both defined as the type cc_t, which I have always seen be an alias for unsigned char (1 byte). 				This puts an upper limit on the number of VMIN characters to be 255 and the maximum timeout of 25.5 seconds (255 				deciseconds).

			“Returning as soon as any data is received” does not mean you will only get 1 byte at a time. Depending on the OS 				latency, serial port speed, hardware buffers and many other things you have no direct control over, you may receive 				any number of bytes.

			For example, if we wanted to wait for up to 1s, returning as soon as any data was received, we could use:*/
			tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
			tty.c_cc[VMIN] = 0;

			/*Baud Rate: */
			/*Rather than use bit fields as with all the other settings, the serial port baud rate is set by calling the functions 				cfsetispeed() and cfsetospeed(), passing in a pointer to your tty struct and a enum: */
			//Setting baud rate to 9600 in and out
			cfsetispeed(&tty,B115200);
			cfsetospeed(&tty,B115200);

			//Compliant enum values:
			/* B0, B50, B75, B110, B134, B150, B200, B300, B600, B1200,
			   B1800, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800*/
			
			// Save tty settings, also checking for error
			if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
			    	printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
				return -1;
			}
			else{
				printf("Setup completed!\n");
				return serial_port;	
			}
						
				
		}
	}
}

short int Identify(const int serial_port){
	char msg[6+1];
	char read_buf[256];

	strcpy(msg,"ID0000");
	short int flag = 0;
	short int index = 0;

	while(index < 30 && !flag){
		write(serial_port, msg, sizeof(msg));
		int n = read(serial_port, &read_buf, sizeof(read_buf));

		//printf("bytes read: %d\n",n);
		if(strstr(read_buf,"OK")) flag = 1;
		//printf("%s\n",read_buf);
		/*for(int i = 0; i < n; i++) printf("%c ",read_buf[i]);
		printf("\n");
		printf("%s\n",read_buf);*/
		index++;
	}

	return flag;
}

uint32_t HowManyMessages(const char * cmd, const int serial_port){

	short int gotcha = 0;
	uint32_t value = -1;
	char read_buf[256];
	short int index = 0;

	while(index < 50 && !gotcha){							
		write(serial_port, cmd, sizeof(cmd));
		int n = read(serial_port, &read_buf, sizeof(read_buf));
		//printf("%s\n",read_buf);
		char * strptr = strstr(read_buf,"QS");
									
		if(strptr && gotcha == 0){
			gotcha = 1;
			strptr+=2;
			printf("\tDevice answers: QS%c%c%c\n",*(strptr),*(strptr+1),*(strptr+2));
			char payload_str[3];
			strncpy(payload_str,strptr+2,3);
			value = atoi(payload_str);
			//for(int i = 0; i < 5; i++) value += atoi(*(strptr+i))*pow(10,i);
		}
							
	}

	return value;
}

uint32_t HowManyBytes(const char * cmd, const int serial_port){

	short int gotcha = 0;
	uint32_t value = -1;
	char read_buf[256];
	short int index = 0;

	while(index < 50 && !gotcha){							
		write(serial_port, cmd, sizeof(cmd));
		int n = read(serial_port, &read_buf, sizeof(read_buf));
		char * strptr = strstr(read_buf,"QB");
									
		if(strptr && gotcha == 0){
			gotcha = 1;
			strptr+=2;
			printf("\tDevice answers: QB%c%c%c%c%c\n",*(strptr),*(strptr+1),*(strptr+2),*(strptr+3),*(strptr+4));
			char payload_str[5];
			strncpy(payload_str,strptr+2,5);
			value = atoi(payload_str);
			//for(int i = 0; i < 5; i++) value += atoi(*(strptr+i))*pow(10,i);
		}

		index++;
							
	}

	return value;
}

uint32_t GetMeBytes(const char * cmd, const int serial_port){

	short int gotcha = 0;
	uint32_t value = -1;
	char read_buf[256];
	short int index = 0;

	while(index < 100 && !gotcha){							
		write(serial_port, cmd, sizeof(cmd));
		int n = read(serial_port, &read_buf, sizeof(read_buf));
		char * strptr = strstr(read_buf,"MB");
									
		if(strptr && gotcha == 0){
			gotcha = 1;
			strptr+=2;
			printf("\tDevice answers: MB%c%c%c%c%c\n",*(strptr),*(strptr+1),*(strptr+2),*(strptr+3),*(strptr+4));
			char payload_str[5];
			strncpy(payload_str,strptr+2,5);
			value = atoi(payload_str);
		}

		index++;
							
	}

	return value;
}

uint32_t GetMeMessages(char * cmd, const int serial_port, const uint32_t size){

	short int gotcha = 0;
	uint32_t value = -1;
	char read_buf[2+ 256*32];
	short int index = 0;
	char msg[6+1];

	//read_buf = (char *)malloc(4*(2+size)*sizeof(char));
	write(serial_port, cmd, sizeof(cmd));
	while(index < 100 && !gotcha){	
						
		int n = read(serial_port, &read_buf, sizeof(read_buf));
		//printf("%s\n",read_buf);
		char * strptr = strstr(read_buf,"MD");
		//printf("BLocking\n");
		
		if(strptr && gotcha == 0){
			//If there is an H it means that the message was not received correctly, so request a resend
			short int H_flag = 0;
			for(int i = 0; i < size; i++) if(*(strptr+2+i) == 'H') H_flag = 1;

			if(!H_flag){
				gotcha = 1;
				strptr+=2;
				printf("\tDevice answers: MD");
				for(int i = 0; i < size; i++) printf("%c",*(strptr+i));
				printf("\n\n");
				//Send MDREAD to device
				strcpy(cmd,"MDR");
				write(serial_port, cmd, sizeof(cmd));
				value = 1;
			}
			else{
				//Send MDRSND to device
				write(serial_port, cmd, sizeof(cmd));			
			}

		}

		
		index++;					
	}

	//free(read_buf);

	return value;
}

int SerialDisconnect(const int serial_port){
	
	//Close communication
	close(serial_port);
	return 1;
}
