/* Includes. */
#include "Devicelib.h"

/* ------- FUNZIONI DEL DOTTORANDO ------- */

void init_queue(struct BufferQueue ** Msg_Queue) {
	*Msg_Queue = (struct BufferQueue *) malloc(sizeof(struct BufferQueue));
	(*Msg_Queue)->size = 0;
	(*Msg_Queue)->bytes = 0;
	(*Msg_Queue)->head = 0;
	(*Msg_Queue)->tail = 0;

	for (int i = 0; i < 256; i++) {
		(*Msg_Queue)->frame[i].ID = 0;
		(*Msg_Queue)->frame[i].DLC = 0;
		for (int j = 0; j < 8; j++)
			(*Msg_Queue)->frame[i].Data[j] = 0;
	}

}

void deinit_queue(struct BufferQueue ** Msg_Queue) {
	free(*Msg_Queue);
}

u8 get_msg_bytes(struct BufferQueue *Q, u8 p) {

	u8 response = 0;

	for (int i = 0; i < p; i++) {
		if ((uint8_t) Q->frame[(Q->head + i) % (QMAXSIZE - 1)].DLC != 0) {
			response += 2; //ID
			response += (uint8_t) Q->frame[(Q->head + i) % (QMAXSIZE - 1)].DLC;
		}

	}

	return response;
}

void get_msg(struct BufferQueue * Q, u8 n, char * m) {

	char hex_ID[4];
	char hex_Data[2];

	for (int i = 0; i < n; i++) {

		sprintf(hex_ID, "%04x",
				(unsigned int) Q->frame[(Q->head + i) % (QMAXSIZE - 1)].ID);
		strcat(m, hex_ID);

		for (int j = 0; j < Q->frame[(Q->head + i) % (QMAXSIZE - 1)].DLC; j++) {
			sprintf(hex_Data, "%02x",
					(uint8_t) Q->frame[(Q->head + i) % (QMAXSIZE - 1)].Data[j]);
			strcat(m, hex_Data);
		}
	}

}

void RX_canframe(volatile unsigned int * SPI_Controller, struct BufferQueue * Q) {

	SPI_Controller[3] = 0x000000a0; //Require status from PmodCAN
	while ((SPI_Controller[5] & 0x03C00000) >> 22 == 0)
		; //Check if there are Received messages on the SPI interface
	SPI_Controller[4];

	if (Q->size != QMAXSIZE) {
		if ((SPI_Controller[0] & 0x00000003) == 0x01
				|| (SPI_Controller[0] & 0x00000003) == 0x03) { //Check if a message is received on the PmodCAN Buffer 0
			RX_message(SPI_Controller, 0, &Q->frame[Q->tail]);
			Q->tail = (Q->tail + 1) % (QMAXSIZE);
			Q->size++;
			Q->bytes = get_msg_bytes(Q, Q->size);
		} else if ((SPI_Controller[0] & 0x00000003) == 0x02) { //Check if a message is received on the PmodCAN Buffer 1
			RX_message(SPI_Controller, 1, &Q->frame[Q->tail]);
			Q->tail = (Q->tail + 1) % (QMAXSIZE);
			Q->bytes = get_msg_bytes(Q, Q->size);
		}
	}

}

void FreeHead(struct BufferQueue * Q) {
	Q->head = (Q->head + 1) % (QMAXSIZE);
	Q->size--;
	Q->bytes = get_msg_bytes(Q, Q->size);
}

void cleanmsg(char * cmd_msg) {
	for (int i = 0; i < 256; i++)
		cmd_msg[i] = ' ';
}

int usleep_A9(uint32_t useconds) {
#if defined (SLEEP_TIMER_BASEADDR)
	Xil_SleepTTCCommon(useconds, COUNTS_PER_USECOND);
#else
	XTime tEnd, tCur;

	XTime_GetTime(&tCur);
	tEnd = tCur + (((XTime) useconds) * COUNTS_PER_USECOND);
	do {
		XTime_GetTime(&tCur);
	} while (tCur < tEnd);
#endif

	return 0;
}

/* ------- FUNZIONI NOSTRE ------- */

//Inizializzazione di un singolo frame CAN
void init_CANframe(struct CANFrame ** frame) {

	/*	*frame = (struct CANFrame*) malloc( sizeof( struct CANFrame ) );		//con la malloc() non funziona!!!!
	 (*frame)->ID = 0;
	 (*frame)->DLC = 8;
	 for( int i=0; i<(*frame)->DLC; i++ ) {
	 (*frame)->Data[i] = 0;
	 }
	 */
	*frame = (struct CANFrame*) pvPortMalloc(sizeof(struct CANFrame));
	(*frame)->ID = 0;
	(*frame)->DLC = 8;
	for (int i = 0; i < (*frame)->DLC; i++) {
		(*frame)->Data[i] = 0;
	}

}
//IMPLEMENTAZIONE UGUALE A COME HA FATTO IL DOTTORANDO (vedere funzione "init_queue()")
//si usa ** perché devi passare il puntatore al puntatore alla struct, poiché non sai anche il puntatore alla struct dove punta, poiché lo sceglie la malloc (se ho ben capito)

//PERCHE' HO USATO "pvPortMalloc" INVECE DI "malloc"?
// https://www.freertos.org/a00111.html

void deinit_CANframe(struct CANFrame ** frame) {

//	free( *frame );															//con la free() non funziona!!!!

	vPortFree(*frame);
}

//Inserimento di un elemento in modo circolare. FORSE NON SERVE PIU'. La sola funzione "xQueueSend()" dovrebbe fornire la stessa funzionalità
void CircularQueueSend(QueueHandle_t queue, struct CANFrame * frame,
		TickType_t blocktime) {

	taskENTER_CRITICAL()
	;

	if (uxQueueSpacesAvailable(queue)) {

		xQueueSend(queue, (void* ) frame, blocktime);
	}

	taskEXIT_CRITICAL()
	;
}
//INFO SULLA CODA CIRCOLARE QUI:
// Come implementare la coda circolare: https://stackoverflow.com/questions/71749307/free-rtos-circular-queue
// La coda di FreeRTOS è già una coda circolare: https://www.freertos.org/FreeRTOS_Support_Forum_Archive/May_2016/freertos_How_to_traverse_a_queue_0be515fcj.html

//Funzione di ricezione di frame CAN dal modulo Pmod CAN
void RX_canframe_2_0(volatile unsigned int * SPI_Controller,
		QueueHandle_t queue, unsigned int* n_lost_frame) {

	struct CANFrame * frame_CAN;

	//Inizializzazione (opzione 1, capire quale funziona e quale no)
	/*	frame_CAN->ID = 0;
	 frame_CAN->DLC = 8;
	 for( int i=0; i<frame_CAN->DLC; i++ ) {
	 frame_CAN->Data[i] = 0;
	 }
	 frame_CAN->DLC = 0;
	 */
	//Inizializzazione (opzione 2, capire quale funziona e quale no)
	init_CANframe(&frame_CAN);

	//Codice di basso livello che usa i registri della periferica custom SPI_Controller
	SPI_Controller[3] = 0x000000a0; //Require status from PmodCAN
	while ((SPI_Controller[5] & 0x03C00000) >> 22 == 0)
		; //Check if there are Received messages on the SPI interface
	SPI_Controller[4];

	if ((SPI_Controller[0] & 0x00000003) == 0x01
			|| (SPI_Controller[0] & 0x00000003) == 0x03) { //Check if a message is received on the PmodCAN Buffer 0

		RX_message(SPI_Controller, 0, frame_CAN);

		if (uxQueueSpacesAvailable(queue)) {
			xQueueSend(queue, (void* ) frame_CAN, 0UL);
		} else {
			(*n_lost_frame)++;
		}
	}

	else if ((SPI_Controller[0] & 0x00000003) == 0x02) { //Check if a message is received on the PmodCAN Buffer 1

		RX_message(SPI_Controller, 1, frame_CAN);		//xQueue->pcWriteTo

		if (uxQueueSpacesAvailable(queue)) {
			xQueueSend(queue, (void* ) frame_CAN, 0UL);
		} else {
			(*n_lost_frame)++;
		}

		xQueueSend(queue, (void* ) frame_CAN, 0UL);
	}

	deinit_CANframe(&frame_CAN);

}
//FORSE MAGGIORI INFO QUI:
// Esercizio di trasmissione e ricezione passando struct e puntatori: https://freertos.org/a00118.html

//Azzeramento di un singolo CAN frame
void zero_CANframe(struct CANFrame * frame) {

	frame->ID = 0;
	frame->DLC = 0;
	for (int i = 0; i < 8; i++) {
		frame->Data[i] = 0;
	}

}

