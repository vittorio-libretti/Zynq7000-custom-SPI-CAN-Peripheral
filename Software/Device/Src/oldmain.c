
/*
 *
 * slvreg0 - 0x43c00000 - 0x43c00003 -- Read last received Register
 * slvreg1 - 0x43c00004 - 0x43c00007 -- Unused
 * slvreg2 - 0x43c00008 - 0x43c0000b -- Unused
 * slvreg3 - 0x43c0000c - 0x43c0000f -- TX write Register
 * slvreg4 - 0x43c00010 - 0x43c00013 -- RX read Register
 * slvreg5 - 0x43c00014 - 0x43c00017 -- Status Register (counters and States)
 * slvreg6 - 0x43c00018 - 0x43c0001b -- TX head debug Register
 * slvreg7 - 0x43c0001c - 0x43c0001f -- Unused
 *
 *
 *
 *
 * */


#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include <xtime_l.h>
#include <time.h>
#include "CustomFilter.h"


int oldmain()
{
    init_platform();

    volatile unsigned int * SPI_Controller = (volatile unsigned int *)0x43c00000;
    struct CustomFilter F;
    struct CANFrame C;
    unsigned int Threshold[8] = {50,60,25,30,100,0,0,0};
    XTime tStart, tEnd;
    XTime tPercStart, tPercEnd;
    XTime StartVector[1000];
    XTime EndVector[1000];
    unsigned char Data[8] = {0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08};
    int i = 0;
    int j = 0;

    //TEST PER LA UART
    int ms = 10000000;

    while(1){
    	/*clock_t timeDelay = ms + clock();
    	printf("Time Delay: %d ",timeDelay);*/
    	while(j < ms) j++;
    	j = 0;
    	printf("Start%d\n",i++);
    }



    //FINE TEST PER LA USART.

    /*Boot(SPI_Controller);
    FilterSetup(&F,0x00150200,0x78c28818,Threshold,0x196c8c8a,0x00000000,0x00000000); //check 5 processing data from id 001.
    F.Time_Window_Counter = 3000000;

    while(F.Time_Window_Counter != 0){

    	SPI_Controller[3] = 0x000000a0; //Require status from PmodCAN
        while((SPI_Controller[5] & 0x03C00000) >> 22 == 0); //Check if there are Received messages on the SPI interface
        SPI_Controller[4];

        if((SPI_Controller[0] & 0x00000003) == 0x01 || (SPI_Controller[0] & 0x00000003) == 0x03){ //Check if a message is received on the PmodCAN Buffer 0
        	RX_message(SPI_Controller,0,&C);
        	FilterRead(&F,C);
        }
        else if((SPI_Controller[0] & 0x00000003) == 0x02){ //Check if a message is received on the PmodCAN Buffer 1
        	RX_message(SPI_Controller,1,&C);
        	FilterRead(&F,C);
        }
        F.Time_Window_Counter--;
     }

     FilterRead(&F,C);
     //TX_message(SPI_Controller,0,0x0020000,0x08,Data);
     if( F.Regfile.Status_register & 00000001 == 0x01 ) TX_message(SPI_Controller,0,0x0000000,0x08,Data);*/

    /*XTime_GetTime(&tStart);

    while(F.Global_Counter != 200){

    	SPI_Controller[3] = 0x000000a0;
    	while((SPI_Controller[5] & 0x03C00000) >> 22 == 0); //Check if there are Received messages
    	SPI_Controller[4];

    	if((SPI_Controller[0] & 0x00000003) == 0x01 || (SPI_Controller[0] & 0x00000003) == 0x03){

    		RX_message(SPI_Controller,0,&C);
    		XTime_GetTime(&StartVector[i]);
    		FilterRead(&F,C);
    		XTime_GetTime(&EndVector[i]);

    		i++;
    	}
    	else if((SPI_Controller[0] & 0x00000003) == 0x02){
    		RX_message(SPI_Controller,1,&C);
    		FilterRead(&F,C);
    	}
    	F.Time_Window_Counter--;
     }

    XTime_GetTime(&tPercStart);
    FilterRead(&F,C);
    XTime_GetTime(&tPercEnd);
    XTime_GetTime(&tEnd);

    printf("Global Counter : %d\n",F.Global_Counter);
   printf("Elapsed Time : %lld\n\r",tEnd-tStart);
   for(i = 0; i < F.Global_Counter-1; i++ ) printf("Elapsed Time %d : %lld\n\r",i,EndVector[i]-StartVector[i]);
   printf("Elapsed Perc Time : %lld\n\r",tPercEnd-tPercStart);*/

    /*int select = 0;
    int reg_1 = 0;
    int reg_2 = 0;
    unsigned int value;*/

    //Boot(SPI_Controller);

    /*while(1){

    	scanf("%d",&select);

    	if(select == 1){
    		printf("Insert value to write :\n");
    		scanf("%08x",&value);
    		printf("Insert reg to write in :\n");
    		scanf("%d",&reg_1);
    		SPI_Controller[reg_1] = value;
    		printf("Writing 0x%08x in reg %d\n",value, reg_1);
    		select = 0;
    		reg_1 = 0;
    	}
    	else if(select == 2){

    		printf("Select reg to read from :\n");
    		scanf("%d",&reg_2);

    		if( reg_2 == 4 ){
    			SPI_Controller[reg_2];
        		printf("Activate Rx from reg 4, then read value from reg 0 : 0x%08x\n", SPI_Controller[0]);
    		}
    		else if( reg_2 == 5 ) ReadAndParseStatus(SPI_Controller);
    		else{
        		printf("read value from reg %d : 0x%08x\n", reg_2, SPI_Controller[reg_2]);
    		}

    		select = 0;
    		reg_2 = 0;
    	}
    	else if(select == 3){
    		TX_message(SPI_Controller,0,0x0000000,0x08,Data);
    	}
    	else if(select == 4){

    		RX_message(SPI_Controller,0,&C);
    		//FilterRead(&F,C);
    		//F.Time_Window_Counter--;
    	}
    	else if(select == 5){
    		FilterSetup(&F,0x00510200,0xb8c28818,Threshold,0x096c8c8a,0x00000000,0x00000000);
    	    F.Time_Window_Counter = 1;
    		//0x096c8c8a : 10%,25%,50%,75%
    	}

    }*/

    cleanup_platform();
    return 0;
}
