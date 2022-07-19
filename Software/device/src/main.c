/* Own includes. */
#include "Devicelib.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Xilinx includes. */
#include "xil_printf.h"			//implementazione della sola funzione "xil_printf" a scopo di debug (invece di includere la definizione della printf C standard)
#include "xparameters.h"
#include "xuartps.h"			//funzioni di inizializzazione, trasmissione e ricezione verso la UART (verso l'host).
#include "platform.h"			//funzioni init_platform() e cleanup_platform() (CODICE DOTTORANDO)

#include "xtime_l.h"
#include "math.h"

/* Defines. */
#define DELAY_1_SECOND		  1000UL
#define QUEUE_SIZE 256
#define NUMBER_OF_MSG_TO_TEST 1000

/* Thread Functions Definition. */
static void talkWithSPICAN_Task(void *pvParameters);
static void talkWithHost_Task(void *pvParameters);

/* Static Variables Definition. */
static TaskHandle_t xTalkWithSPICAN_Task;
static TaskHandle_t xTalkWithHost_Task;
static QueueHandle_t xQueue = NULL;

XUartPs_Config *Config_1;
XUartPs Uart_PS_1;

volatile unsigned int * SPI_Controller = (volatile unsigned int *) 0x43c00000; //puntatore alla periferica custom SPI Controller
unsigned int n_msg;	//indica il numero di messaggi CAN presenti nella coda
unsigned int frame_lost = 0;	//indica il numero di messaggi CAN che vengono persi quando la coda è piena
unsigned int * n_frame_lost = &frame_lost;

//VARIABILI PER IL TEMPO
uint32_t test_time;

XTime gbl_time_before_test;
XTime *p_gbl_time_before_test = &gbl_time_before_test;

XTime gbl_time_after_test;
XTime *p_gbl_time_after_test = &gbl_time_after_test;

float duration = 0;

int main(void) {

	//Inizializzazione periferia UART
	init_platform();
	int Status;
	Config_1 = XUartPs_LookupConfig(XPAR_PS7_UART_1_DEVICE_ID);
	if (Config_1 == NULL) {
		return XST_FAILURE;
	}
	Status = XUartPs_CfgInitialize(&Uart_PS_1, Config_1, Config_1->BaseAddress); //in questa funzione si può cambiare il baud rate (e forse il bit di parità)
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	Status = XUartPs_SelfTest(&Uart_PS_1);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	//Inizializzazione periferica custom su fpga
	Boot(SPI_Controller);

	n_msg = 0;

	xTaskCreate(talkWithSPICAN_Task, /* The function that implements the task. */
	(const char *) "talkWithSPICAN_Task", /* Text name for the task, provided to assist debugging only. */
	configMINIMAL_STACK_SIZE, /* The stack allocated to the task. */
	NULL, /* The task parameter is not used, so set to NULL. */
	tskIDLE_PRIORITY + 1, /* The task runs at the idle priority. */
	&xTalkWithSPICAN_Task);

	xTaskCreate(talkWithHost_Task, (const char *) "talkWithHost_Task",
	configMINIMAL_STACK_SIZE,
	NULL,
	tskIDLE_PRIORITY + 1, &xTalkWithHost_Task);

	//coda circolare di FreeRTOS
	xQueue = xQueueCreate(QUEUE_SIZE,				//la coda ha 256 elementi
			sizeof(struct CANFrame));//ogni elemento ha dimensione (32 + 8 + 8 = 48 byte)

	/* Check the queue was created. */
	configASSERT(xQueue);

	/* Start the tasks and timer running. */
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following line
	 will never be reached.  If the following line does execute, then there was
	 insufficient FreeRTOS heap memory available for the idle and/or timer tasks
	 to be created.  See the memory management section on the FreeRTOS web site
	 for more details. */
	for (;;)
		;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*----------  TASK VERSO PERIFERICA CUSTOM SPI_Controller  ------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

static void talkWithSPICAN_Task(void *pvParameters) {

	XTime_GetTime(p_gbl_time_before_test);
	int i = 0;
	for (;;) {

		RX_canframe_2_0(SPI_Controller, xQueue, n_frame_lost);

		//Codice per misurare il tempo necessario affinchè la coda sia piena
//		if (((unsigned int) uxQueueMessagesWaiting(xQueue)) == 256 && i == 0) {
//			XTime_GetTime(p_gbl_time_after_test);
//			printf("Mex in coda: %d \n",
//					(unsigned int) uxQueueMessagesWaiting(xQueue));
//			float duration01 = (float) (gbl_time_after_test
//					- gbl_time_before_test) / (COUNTS_PER_SECOND);
//			printf("Durata %.8f \n ", duration01);
//			i = 1;
//
//		}

	}

}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*--------------------------  TASK VERSO HOST  ------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

static void talkWithHost_Task(void *pvParameters) {
	const TickType_t x1second = pdMS_TO_TICKS(DELAY_1_SECOND);
	vTaskDelay(x1second);				//ritardo di 1s per far in modo che parta prima il task che parla con la Zybo

	const unsigned char HEARTHBEAT[8] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8 };
	const unsigned char ZERO[8] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

	struct CANFrame * frame_recd;

	init_CANframe(&frame_recd);

	char choice = 0;

	//Allocazione dinamica frameBuffer
	int frameB_size_max = 64;	//limite fisico del buffer della UART della Zybo
	struct CANFrameToSend * frameBuffer = (struct CANFrameToSend *) pvPortMalloc(sizeof(frameB_size_max));

	for (;;) {

		XUartPs_Recv(&Uart_PS_1, &choice, 1);

		switch (choice) {
		case '0': //identificazione della porta su cui è connessa la Zybo

			XUartPs_Send(&Uart_PS_1, "OK", 2);
			choice = 0;
			break;
		case '1': //restituisce l'hearthbeat

			while (memcmp((unsigned char *) (frame_recd->Data), (unsigned char *) HEARTHBEAT, 8)) {
				xQueueReceive(xQueue, (void*) frame_recd, portMAX_DELAY);
			}

			for (int j = 0; j < (frame_recd->DLC); j++) {
				xil_printf("%x", frame_recd->Data[j]);
			}

			choice = 0;
			zero_CANframe(frame_recd);
			break;

		case '2': // restituisce la prima misura di temperatura e pressione disponibile

			while (!memcmp((unsigned char *) (frame_recd->Data), (unsigned char *) HEARTHBEAT, 8)
					|| !memcmp((unsigned char *) (frame_recd->Data), (unsigned char *) ZERO, 8) || (frame_recd->Data[3] == 0x0 && frame_recd->Data[4] == 0x0)) {

				xQueueReceive(xQueue, (void*) frame_recd, portMAX_DELAY);
			}

			while (XUartPs_IsSending(&Uart_PS_1))
				;
			XUartPs_Send(&Uart_PS_1, (u8 *) frame_recd, sizeof(struct CANFrame));
			choice = 0;
			zero_CANframe(frame_recd);
			break;

		case '3': // restituisce il numero di messaggi persi

			while (XUartPs_IsSending(&Uart_PS_1))
				;
			XUartPs_Send(&Uart_PS_1, (u8 *) n_frame_lost, sizeof(n_frame_lost));
			choice = 0;
			break;

		case '4': // restituisce il numero di frame nella coda

			n_msg = (unsigned int) uxQueueMessagesWaiting(xQueue);
			unsigned int * n_msg_ptr = &n_msg;
			XUartPs_Send(&Uart_PS_1, n_msg_ptr, sizeof(n_msg)); //4 byte
			//printf("Mex in coda: %d \n", n_msg);

			choice = 0;
			break;

		case '5': // restituisce i primi X messaggi dalla coda

			//Inizializzazione a zero frameBuffer
			for (int i = 0; i < 4; i++) {

				//Riempimento frameBuffer
				(frameBuffer + i)->ID = 0;
				(frameBuffer + i)->DLC = 8;
				for (int j = 0; j < (frameBuffer + i)->DLC; j++) {
					(frameBuffer + i)->Data[j] = 0;
				}
				(frameBuffer + i)->CTRL1 = 0;
				(frameBuffer + i)->CTRL2 = 0;
				(frameBuffer + i)->CTRL3 = 0;
			}

			//variabili d'appoggio
			int n_msg_to_send = n_msg;
			int cont = 0;
			int n_byte_to_send = 0;

			for (int i = 0; i < n_msg; i++) {

				//Estrazione messaggio dalla coda
				xQueueReceive(xQueue, (void*) frame_recd, portMAX_DELAY);

				//Riempimento frameBuffer
				int ofs = i % 4;
				(frameBuffer + ofs)->ID = frame_recd->ID;

				(frameBuffer + ofs)->DLC = frame_recd->DLC;
				for (int j = 0; j < (frameBuffer + ofs)->DLC; j++) {
					(frameBuffer + ofs)->Data[j] = frame_recd->Data[j];
				}
				(frameBuffer + ofs)->CTRL1 = 0xdd;
				(frameBuffer + ofs)->CTRL2 = 0xee;
				(frameBuffer + ofs)->CTRL3 = 0xff;

				//logica di invio del frameBuffer
				cont++;
				if (cont == 4 || n_msg_to_send == cont) {

					if (n_msg_to_send >= 4) {
						n_byte_to_send = 64;
						n_msg_to_send = n_msg_to_send - 4;
						while (XUartPs_IsSending(&Uart_PS_1)) {
						}

						XUartPs_Send(&Uart_PS_1, (u8 *) frameBuffer, n_byte_to_send);

					} else {
						n_byte_to_send = n_msg_to_send * 16;
						n_msg_to_send = 0;
						while (XUartPs_IsSending(&Uart_PS_1)) {
						}
						XUartPs_Send(&Uart_PS_1, (u8 *) frameBuffer, n_byte_to_send);
					}

					//Azzeramento frameBuffer (per il prossimo invio)
					zero_frameBuffer(frameBuffer);
					cont = 0;
				}

				//Azzeramento frame_recd (per la prossima estrazione)
				zero_CANframe(frame_recd);
			}

			n_msg = 0;
			choice = 0;
			break;

		case '6':	//invio di NUMBER_OF_MSG_TO_TEST frame per analisi

			//Inizializzazione a zero frameBuffer
			for (int i = 0; i < 4; i++) {

				//Riempimento frameBuffer
				(frameBuffer + i)->ID = 0;
				(frameBuffer + i)->DLC = 8;
				for (int j = 0; j < (frameBuffer + i)->DLC; j++) {
					(frameBuffer + i)->Data[j] = 0;
				}
				(frameBuffer + i)->CTRL1 = 0;
				(frameBuffer + i)->CTRL2 = 0;
				(frameBuffer + i)->CTRL3 = 0;
			}

			//variabili d'appoggio
			int n_msg_to_send2 = NUMBER_OF_MSG_TO_TEST;
			int cont2 = 0;
			int n_byte_to_send2 = 0;

			//start
			//XTime_GetTime(p_gbl_time_before_test);

			//per ogni messaggio da inviare all'host...
			for (int i = 0; i < NUMBER_OF_MSG_TO_TEST; i++) {

				//Riempimento frameBuffer
				int ofs = i % 4;
				(frameBuffer + ofs)->ID = i/*0x7ea*/;
				;		//frame_recd->ID;
				(frameBuffer + ofs)->DLC = 8;	//frame_recd->DLC;
				for (int j = 0; j < (frameBuffer + ofs)->DLC; j++) {
					(frameBuffer + ofs)->Data[j] = HEARTHBEAT[j];	//frame_recd->Data[j];
				}
				(frameBuffer + ofs)->CTRL1 = 0xdd; //abbiamo modificato i byte di controllo, differenziandoli
				(frameBuffer + ofs)->CTRL2 = 0xee;
				(frameBuffer + ofs)->CTRL3 = 0xff;

				//logica di invio del frameBuffer
				cont2++;
				if (cont2 == 4 || n_msg_to_send2 == cont2) {

					if (n_msg_to_send2 >= 4) {
						n_byte_to_send2 = 64;
						n_msg_to_send2 = n_msg_to_send2 - 4;
						while (XUartPs_IsSending(&Uart_PS_1)) {
						}

						XUartPs_Send(&Uart_PS_1, (u8 *) frameBuffer, n_byte_to_send2);
						//Questa sleep è necessaria per sincronizzare la velocità con l'host e ricevere tutti i messaggi in maniera corretta
						usleep(100000);

					} else {
						n_byte_to_send2 = n_msg_to_send2 * 16;
						n_msg_to_send2 = 0;
						while (XUartPs_IsSending(&Uart_PS_1)) {
						}
						XUartPs_Send(&Uart_PS_1, (u8 *) frameBuffer, n_byte_to_send2);
                        //printf("frame inviato %u\n", i);
					}

					//Azzeramento frameBuffer (per il prossimo invio)
					zero_frameBuffer(frameBuffer);
					cont2 = 0;
				}

				//Azzeramento frame_recd (per la prossima estrazione)
				zero_CANframe(frame_recd);
			}

			//STOP
			//MISURA DEL TEMPO NECESSARIO AD INVIARE LATO HOST TUTTI I MESSAGGI NELLA CODA
//			XTime_GetTime(p_gbl_time_after_test);
//
//			printf("Start %lld \n", (u64) gbl_time_before_test);
//			printf("Stop  %.8f \n",
//					(float) gbl_time_after_test / (COUNTS_PER_SECOND));
//
//			duration = (float) (gbl_time_after_test - gbl_time_before_test)
//					/ (COUNTS_PER_SECOND);
//
//			printf("Durata %.8f \n ", duration);

			choice = 0;
			break;

		default:

			break;

		}

	}

}
