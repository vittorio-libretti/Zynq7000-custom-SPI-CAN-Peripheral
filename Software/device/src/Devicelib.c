/* Includes. */
#include "Devicelib.h"

//Inizializzazione di un singolo frame CAN
void init_CANframe(struct CANFrame ** frame) {

	*frame = (struct CANFrame*) pvPortMalloc(sizeof(struct CANFrame));
	(*frame)->ID = 0;
	(*frame)->DLC = 8;
	for (int i = 0; i < (*frame)->DLC; i++) {
		(*frame)->Data[i] = 0;
	}

}

void deinit_CANframe(struct CANFrame ** frame) {
	vPortFree(*frame);
}

//Funzione di ricezione di frame CAN dal modulo Pmod CAN
void RX_canframe_2_0(volatile unsigned int * SPI_Controller, QueueHandle_t queue, unsigned int* n_lost_frame) {

	struct CANFrame * frame_CAN;

	//Inizializzazione (opzione 2, capire quale funziona e quale no)
	init_CANframe(&frame_CAN);

	//Codice di basso livello che usa i registri della periferica custom SPI_Controller
	SPI_Controller[3] = 0x000000a0; //Require status from PmodCAN
	while ((SPI_Controller[5] & 0x03C00000) >> 22 == 0)
		; //Check if there are Received messages on the SPI interface
	SPI_Controller[4];

	if ((SPI_Controller[0] & 0x00000003) == 0x01 || (SPI_Controller[0] & 0x00000003) == 0x03) { //Check if a message is received on the PmodCAN Buffer 0

		RX_message(SPI_Controller, 0, frame_CAN);

		if (uxQueueSpacesAvailable(queue)) {
			xQueueSend(queue, (void* ) frame_CAN, 0UL);
		} else {
			(*n_lost_frame)++;
		}
	}

	else if ((SPI_Controller[0] & 0x00000003) == 0x02) { //Check if a message is received on the PmodCAN Buffer 1

		RX_message(SPI_Controller, 1, frame_CAN);

		if (uxQueueSpacesAvailable(queue)) {
			xQueueSend(queue, (void* ) frame_CAN, 0UL);
		} else {
			(*n_lost_frame)++;
		}

		xQueueSend(queue, (void* ) frame_CAN, 0UL);
	}

	deinit_CANframe(&frame_CAN);

}

//Azzeramento di un singolo CAN frame
void zero_CANframe(struct CANFrame * frame) {

	frame->ID = 0;
	frame->DLC = 0;
	for (int i = 0; i < 8; i++) {
		frame->Data[i] = 0;
	}

}

//Azzeramento del frameBuffer
void zero_frameBuffer(struct CANFrameToSend * frameBuffer) {

	int n_msg_max = 4;
	for (int i = 0; i < n_msg_max; i++) {

		//Riempimento frameBuffer
		(frameBuffer + i)->ID = 0;
		(frameBuffer + i)->DLC = 0;
		for (int j = 0; j < 8; j++) {
			(frameBuffer + i)->Data[j] = 0;
		}
		(frameBuffer + i)->CTRL1 = 0;
		(frameBuffer + i)->CTRL2 = 0;
		(frameBuffer + i)->CTRL3 = 0;
	}

}

