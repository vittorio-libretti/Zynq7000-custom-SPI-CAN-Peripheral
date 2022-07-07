
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xuartps.h"
#include <xtime_l.h>
#include "Devicelib.h"

XUartPs_Config *Config_1;
XUartPs Uart_PS_1;

int main()
{
    init_platform();
    volatile unsigned int * SPI_Controller = (volatile unsigned int *)0x43c00000;
    struct BufferQueue * Msg_Queue;

    int Status;
    u8 cmd_msg[256];

    //Uart configuration:
    //UART1 is connected to the USB port, UART0 to the pmod port. The setup is made in order to
    //use the Receive function. While the send function does exist, UART1 uses by default printf as well.

    Config_1 = XUartPs_LookupConfig(XPAR_PS7_UART_1_DEVICE_ID);
    if (Config_1 == NULL) {
    	return XST_FAILURE;
    }
    Status = XUartPs_CfgInitialize(&Uart_PS_1, Config_1, Config_1->BaseAddress);
    if (Status != XST_SUCCESS){
    	return XST_FAILURE;
    }

    //PMOD SPI Config: The Boot routine initializes the CANpmod registers and mode.

    Boot(SPI_Controller);


    init_queue(&Msg_Queue);
    Status = IDLE;

    //The cleanmsg function clears the string it receives.
    cleanmsg(cmd_msg);

    //Main loop

    while(1){

    	usleep_A9(100); //Delay of 100us.
    	XUartPs_Recv(&Uart_PS_1, cmd_msg, (NUM_OF_BYTE));

    	usleep_A9(100); //Delay of 100us.

    	if(strstr((char *)cmd_msg, "ID")) {
        	XUartPs_Send(&Uart_PS_1,"OK", 2);
    		//printf("OK");
    	    cleanmsg(cmd_msg);
    	}
		
    	if(strstr((char *)cmd_msg, "MDR")){ //Message data read: a frame was read succesfully
    		//When the host has succesfully read the message, it will send a Message Data Read message.
    		//If that happens the head of the queue is release. This implies that the host will send N
    		//MDR, where MDR is the number of messages read.

    		XUartPs_Send(&Uart_PS_1,"MRMVD",5); //Send Message Removed
    		FreeHead(Msg_Queue);
    		cleanmsg(cmd_msg);
    	}
    	else if(strstr((char *)cmd_msg, "HMM")) { //How Many messages? H M M 0 0 0

    		char m[5] = "QS";
    		char p[3];

    		sprintf(p, "%03u", Msg_Queue ->size);

    		XUartPs_Send(&Uart_PS_1,strcat(m,p),5);
    		//printf("QS%03u",Msg_Queue ->size); //5 bytes response (e.g. : QS020)
    	    cleanmsg(cmd_msg);
    	}
    	else if(strstr((char *)cmd_msg, "HMB")){ //How Many bytes? H M B 0 0 0
    		//Each message contains a 2 Bytes (4 Nibbles) maximum ID and a 8 bytes maximum Payload.
    	    //Each Nibble is sent as a character on to the serial (1 byte), then we have Nibble->Byte. Therefore
    	    //we have to multiply the number of bytes by two.
    		char m[7] = "QB";
    		char p[5];

    		sprintf(p, "%05u", 2*Msg_Queue->bytes);
    		XUartPs_Send(&Uart_PS_1,strcat(m,p),7);
    	    cleanmsg(cmd_msg);
    	}
    	else if(strstr((char *)cmd_msg, "GMB")){ //Get Me Bytes X Y Z (e.g. : GMB122 get me the number of bytes for the first 122 messages

    		char m[7] = "MB";
    		char p[5];

    		sprintf(p, "%05u", 2*get_msg_bytes(Msg_Queue,1));
    		XUartPs_Send(&Uart_PS_1,strcat(m,p),7);

    	    cleanmsg(cmd_msg);
    	    }
    	else if(strstr((char *)cmd_msg, "GMM")){ //Get Me Message allows for reading the first queue message
    		char * strptr = strstr((char *)cmd_msg, "GMM");
    	    char * datamsg;
    	    u16 datasize = 0;
    	    char payload_str[3];

    	    char m[256] = "MD";

    	    strptr+=3;
    	    strncpy(payload_str, strptr , 3);

    	    datasize = 2*get_msg_bytes(Msg_Queue,1);

    	    if(datasize != 0) {
    	    	datamsg = (char *)malloc(datasize);
    	    	strcpy(datamsg,"");
    	    	get_msg(Msg_Queue,1,datamsg);
    	    	XUartPs_Send(&Uart_PS_1,strcat(m,datamsg),2+datasize);
    	        free(datamsg);
    	    }
    	    else XUartPs_Send(&Uart_PS_1,"MD",2);

    	    cleanmsg(cmd_msg);
    	}

    	RX_canframe(SPI_Controller,Msg_Queue);

    }

    deinit_queue(&Msg_Queue);
    cleanup_platform();
    return 0;
}
