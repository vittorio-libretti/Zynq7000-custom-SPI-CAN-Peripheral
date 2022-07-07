#include <stdio.h>
#include "xil_printf.h"

struct CANFrame{
	unsigned int ID;
	unsigned char DLC;
	unsigned char Data[8];
};

void ReadAndParseStatus( volatile unsigned int * SPI_Controller );

void Boot( volatile unsigned int * SPI_Controller );

void TX_message( volatile unsigned int * SPI_Controller, unsigned short Buffer,unsigned int ID, unsigned char DLC, unsigned char * Data);

void RX_message( volatile unsigned int * SPI_Controller, unsigned short Buffer,struct CANFrame * C);
