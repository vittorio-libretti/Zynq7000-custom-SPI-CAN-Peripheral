#include "Devicelib.h"

void init_queue(struct BufferQueue ** Msg_Queue){
	*Msg_Queue = (struct BufferQueue *)malloc(sizeof(struct BufferQueue));
	(*Msg_Queue)->size = 0;
	(*Msg_Queue)->bytes = 0;
	(*Msg_Queue)->head = 0;
	(*Msg_Queue)->tail = 0;

	for(int i = 0; i < 256; i++){
		(*Msg_Queue)->frame[i].ID = 0;
		(*Msg_Queue)->frame[i].DLC = 0;
		for(int j = 0; j < 8; j++) (*Msg_Queue)->frame[i].Data[j] = 0;
	}

}

void deinit_queue(struct BufferQueue ** Msg_Queue){
	free(*Msg_Queue);
}

u8 get_msg_bytes(struct BufferQueue *Q,u8 p){

	u8 response = 0;

	for(int i = 0; i < p; i++){
		if((uint8_t)Q->frame[(Q->head+i)%(QMAXSIZE-1)].DLC != 0){
			response += 2; //ID
			response += (uint8_t)Q->frame[(Q->head+i)%(QMAXSIZE-1)].DLC;
		}

	}

	return response;
}

void get_msg(struct BufferQueue * Q, u8 n, char * m){

	char hex_ID[4];
	char hex_Data[2];

	for(int i = 0; i < n; i++){

		sprintf(hex_ID, "%04x", (unsigned int)Q->frame[(Q->head+i)%(QMAXSIZE-1)].ID);
		strcat(m,hex_ID);

		for(int j = 0; j < Q->frame[(Q->head+i)%(QMAXSIZE-1)].DLC; j++){
			sprintf(hex_Data, "%02x", (uint8_t)Q->frame[(Q->head+i)%(QMAXSIZE-1)].Data[j]);
			strcat(m,hex_Data);
		}
	}

}

void RX_canframe(volatile unsigned int * SPI_Controller, struct BufferQueue * Q){

	SPI_Controller[3] = 0x000000a0; //Require status from PmodCAN
	while((SPI_Controller[5] & 0x03C00000) >> 22 == 0); //Check if there are Received messages on the SPI interface
	SPI_Controller[4];

	if(Q->size != QMAXSIZE){
		if((SPI_Controller[0] & 0x00000003) == 0x01 || (SPI_Controller[0] & 0x00000003) == 0x03){ //Check if a message is received on the PmodCAN Buffer 0
			RX_message(SPI_Controller,0,&Q->frame[Q->tail]);
			Q->tail = (Q->tail + 1)%(QMAXSIZE);
			Q->size++;
			Q->bytes = get_msg_bytes(Q,Q->size);

		}
		else if((SPI_Controller[0] & 0x00000003) == 0x02){ //Check if a message is received on the PmodCAN Buffer 1
			RX_message(SPI_Controller,1,&Q->frame[Q->tail]);
			Q->tail = (Q->tail + 1)%(QMAXSIZE);
			Q->bytes = get_msg_bytes(Q,Q->size);
		}
	}


}

void FreeHead(struct BufferQueue * Q){
	Q->head = (Q->head + 1)%(QMAXSIZE);
	Q->size--;
	Q->bytes = get_msg_bytes(Q,Q->size);
}

void cleanmsg(char * cmd_msg){
	for(int i = 0; i < 256; i++) cmd_msg[i] = ' ';
}

int usleep_A9(uint32_t useconds)
{
#if defined (SLEEP_TIMER_BASEADDR)
	Xil_SleepTTCCommon(useconds, COUNTS_PER_USECOND);
#else
	XTime tEnd, tCur;

	XTime_GetTime(&tCur);
	tEnd = tCur + (((XTime) useconds) * COUNTS_PER_USECOND);
	do
	{
		XTime_GetTime(&tCur);
	} while (tCur < tEnd);
#endif

	return 0;
}
