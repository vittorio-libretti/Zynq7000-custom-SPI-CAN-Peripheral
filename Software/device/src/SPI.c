#include "SPI.h"


void Boot( volatile unsigned int * SPI_Controller ){

	unsigned short size = 81;
	int i = 0;
	int restValue = 0;
	unsigned int ROM[81] = {
			//Reset
			0x000000C0,
			//Set Configuration Mode
			0x80800F05,
			//The following three commands set a CAN speed of 250 kBPS with a CAN clock of 20 MHz.
			0x00412A02,
			0x00FB2902,
			0x00862802,
			//Set the receive filters to either standard or extended identifiers
			0x00000002,
			0x00000102,
			0x00000202,
			0x00000302,
			0x00000402,
			0x00000502,
			0x00000602,
			0x00000702,
			0x00000802,
			0x00000902,
			0x00000A02,
			0x00000B02,
			0x00001002,
			0x00001102,
			0x00001202,
			0x00001302,
			0x00001402,
			0x00001502,
			0x00001602,
			0x00001702,
			0x00001802,
			0x00001902,
			0x00001A02,
			0x00001B02,
			0x00002002,
			0x00002102,
			0x00002202,
			0x00002302,
			0x00002402,
			0x00002502,
			0x00002602,
			0x00002702,
			 //The transmit register flags and settings are also all cleared by setting these register
			0x00003002,
			0x00003102,
			0x00003202,
			0x00003302,
			0x00003402,
			0x00003502,
			0x00003602,
			0x00003702,
			0x00003802,
			0x00003902,
			0x00003A02,
			0x00003B02,
			0x00003C02,
			0x00003D02,
			0x00004002,
			0x00004102,
			0x00004202,
			0x00004302,
			0x00004402,
			0x00004502,
			0x00004602,
			0x00004702,
			0x00004802,
			0x00004902,
			0x00004A02,
			0x00004B02,
			0x00004C02,
			0x00004D02,
			0x00005002,
			0x00005102,
			0x00005202,
			0x00005302,
			0x00005402,
			0x00005502,
			0x00005602,
			0x00005702,
			0x00005802,
			0x00005902,
			0x00005A02,
			0x00005B02,
			0x00005C02,
			0x00005D02,
			//Set the CAN mode for any message type
			0x60646005,
			//Set CAN control mode to normal mode
			0x00800F05,
			//Set CAN control mode to loopback mode 010
			//0x40C00F05
	};

	//printf("Booting SPI-pmodCAN System.....\n");
	for(i = 0; i < size; i++){
		//printf("Ok\n");
		if( i >= 16 ){
			while(restValue < 1) restValue = 15 - ((SPI_Controller[5] & 0x003C0000) >> 18);
			SPI_Controller[3] = ROM[i];
			for(int i = 0; i < 100000; i++) ;
			//printf("Writing instruction number %d from ROM = 0x%08x\n",i,ROM[i]);
		}
		else{
			SPI_Controller[3] = ROM[i];
			for(int i = 0; i < 100000; i++) ;
			//printf("Writing instruction number %d from ROM = 0x%08x\n",i,ROM[i]);
		}

	}

	//printf("Boot complete.\n");

}

void TX_message( volatile unsigned int * SPI_Controller, unsigned short Buffer, unsigned int ID, unsigned char DLC, unsigned char * Data){

	unsigned int Start_Address;
	unsigned short Size;
	unsigned short shift_value;
	int i = 0;

	//Load data through a Load TX Buffer SPI command. I'm using WRITE command anyway

	Start_Address = 0x31 + Buffer*0x10;
	printf("Starting at Address : 0x%08x\n", Start_Address);

	//31,41,51 to 34,44,54 -- ID

	for(i = 0; i < 4; i++){
		shift_value = (8*(3-i));
		SPI_Controller[3] = 0x00000002 + (Start_Address << 8 ) + (((ID >> shift_value) & 0x000000ff) << 16);
		printf("TX at Address : 0x%08x - value : 0x%08x\n", Start_Address, SPI_Controller[6]);
		Start_Address++;
	}

	//35,45,55 -- DLC

	SPI_Controller[3] = 0x00000002 + (Start_Address << 8 ) + ((DLC) << 16);
	printf("TX at Address : 0x%08x - value : 0x%08x\n", Start_Address, SPI_Controller[6]);
	Start_Address++;

	//36,46,56 on -- Data
	Size = (DLC & 0x0f);

	for(i = 0; i < Size; i++){
		SPI_Controller[3] = 0x00000002 + (Start_Address << 8 ) + (Data[i] << 16);
		printf("TX at Address : 0x%08x - value : 0x%08x\n", Start_Address, SPI_Controller[6]);
		Start_Address++;
	}

	//Send a Request-To-Send SPI command for one or more of the three registers of interest

	switch(Buffer){
		case 0 : SPI_Controller[3] = 0x00000081; break;
		case 1 : SPI_Controller[3] = 0x00000082; break;
		case 2 : SPI_Controller[3] = 0x00000084; break;
	}

	printf("Request to Send SPI from buffer %d\n",Buffer);

}

void RX_message( volatile unsigned int * SPI_Controller, unsigned short Buffer,struct CANFrame * C){

	unsigned int Start_Address = 0x00000000;
	unsigned int Data1 = 0x00000000;
	unsigned int Data2 = 0x00000000;
	//unsigned char Data[8] ;
	short int i = 0;

	Start_Address = 0x61 + Buffer*0x10;
	//printf("Starting at Address : 0x%08x\n", Start_Address);

	//Read ID

	SPI_Controller[3] = 0x00000003 + (Start_Address << 8 );
	while((SPI_Controller[5] & 0x03C00000) >> 22 == 0);
	SPI_Controller[4];
	C->ID = (SPI_Controller[0] & 0xffe00000)>>21;
	//printf("CAN ID : 0x%03x\n", C->ID);

	Start_Address += 4;

	SPI_Controller[3] = 0x00000003 + (Start_Address << 8 );
	while((SPI_Controller[5] & 0x03C00000) >> 22 == 0);
	SPI_Controller[4];
	C->DLC = ((SPI_Controller[0] & 0x0f000000) >> 24);
	//printf("DLC : 0x%01x\n", C->DLC);

	Start_Address++;

	SPI_Controller[3] = 0x00000003 + (Start_Address << 8 );
	while((SPI_Controller[5] & 0x03C00000) >> 22 == 0);
	SPI_Controller[4];
	Data1 = SPI_Controller[0];

	Start_Address += 4;

	SPI_Controller[3] = 0x00000003 + (Start_Address << 8 );
	while((SPI_Controller[5] & 0x03C00000) >> 22 == 0);
	SPI_Controller[4];
	Data2 = SPI_Controller[0];

	for( i = 0; i < C->DLC; i++){
		if( i < 4 ) C->Data[i] = ( (Data1 & (0xff000000 >> 8*i)) >> (8*(3-i)) );
		else C->Data[i] = ((Data2 & (0xff000000 >> 8*(i-4))) >> (8*(7-i)) );

	}

	/*printf("Data : 0x");
	for(i = 0; i < C->DLC; i++) printf("%02x",C->Data[i]);
	printf("\n");*/

	//Reset the flag "Buffer" in register 2C

	if( Buffer == 0 ) SPI_Controller[3] = 0x00012C05;
	else if( Buffer == 1 ) SPI_Controller[3] = 0x00022C05;

	//printf("RX Completed\n");
}


void ReadAndParseStatus( volatile unsigned int * SPI_Controller ){

	unsigned int Status = SPI_Controller[5];

	printf("Status Register:  0x%08x\n", Status);

	printf("TX SPI Status :");
	switch(Status & 0x00000007){
		case 0x0: printf(" 000 - IDLE\n"); break;
		case 0x1: printf(" 001 - FETCH\n"); break;
		case 0x2: printf(" 010 - EN_REG\n"); break;
		case 0x3: printf(" 011 - PREP_TO_DECODE\n"); break;
		case 0x4: printf(" 100 - DECODE\n"); break;
		case 0x5: printf(" 101 - CHECK_RX_FULL\n"); break;
		case 0x6: printf(" 110 - PREP_TO_EXECUTE\n"); break;
		case 0x7: printf(" 111 - EXECUTE\n"); break;
	}

	printf("RX SPI Status :");
	switch( (Status & 0x00000038) >> 3){
		case 0x0: printf(" 000 - IDLE\n"); break;
		case 0x1: printf(" 001 - WAIT FOR RX\n"); break;
		case 0x2: printf(" 010 - EN_REG\n"); break;
		case 0x3: printf(" 011 - PUSH\n"); break;
	}

	printf("TX AXI Status :");
	switch( (Status & 0x000001C0) >> 6){
		case 0x0: printf(" 000 - IDLE\n"); break;
		case 0x1: printf(" 001 - FIFOWRITE\n"); break;
		case 0x2: printf(" 010 - REGWRITE\n"); break;
		case 0x3: printf(" 011 - FETCH AND INCREMENT\n"); break;
		case 0x4: printf(" 100 - AXI_RESP\n"); break;
		case 0x5: printf(" 101 - WAIT FOR AXI_ACK\n"); break;
	}

	printf("RX AXI Status : ");
	switch( (Status & 0x00000E00) >> 9){
		case 0x0: printf(" 000 - IDLE\n"); break;
		case 0x1: printf(" 001 - FIFOREAD\n"); break;
		case 0x6: printf(" 110 - REGREAD\n"); break;
		case 0x3: printf(" 011 - FETCH AND DECREMENT\n"); break;
		case 0x4: printf(" 100 - AXI_RESP\n"); break;
		case 0x5: printf(" 101 - WAIT FOR AXI_ACK\n"); break;
	}

	printf("TX STRG TO SPI Status :");
	switch( (Status & 0x00007000) >> 12){
		case 0x0: printf(" 000 - IDLE\n"); break;
		case 0x1: printf(" 001 - FETCH AND DECREMENT\n"); break;
		case 0x2: printf(" 010 - AXI_TO_SPI\n"); break;
	}

	printf("RX STRG TO SPI Status :");
	switch( (Status & 0x00038000) >> 15){
		case 0x0: printf(" 000 - IDLE\n"); break;
		case 0x1: printf(" 001 - FETCH AND INCREMENT\n"); break;
		case 0x2: printf(" 010 - AXI_TO_SPI\n"); break;
	}

	printf("TX Counter : %d\n", (Status & 0x003C0000) >> 18);

	printf("RX Counter : %d\n", (Status & 0x03C00000) >> 22);

	printf("SPI_RX1BF : %d\t SPI_RX2BF : %d\t SPI_INT : %d\n", (Status & 0x04000000)>> 26,(Status & 0x08000000)>>27,(Status & 0x10000000)>>28);

}

