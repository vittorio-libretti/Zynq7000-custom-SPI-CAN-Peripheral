#include <stdio.h>
#include "xil_printf.h"

struct CANFrame{
	unsigned int ID;				/* 32 bit */
	unsigned char DLC;				/*  8 bit */
	unsigned char Data[8];			/*  8 bit */
};

void ReadAndParseStatus( volatile unsigned int * SPI_Controller );

void Boot( volatile unsigned int * SPI_Controller );

void TX_message( volatile unsigned int * SPI_Controller, unsigned short Buffer,unsigned int ID, unsigned char DLC, unsigned char * Data);

void RX_message( volatile unsigned int * SPI_Controller, unsigned short Buffer,struct CANFrame * C);
