/*---------------------------------------------------------------------------*/
/*--------------------------  DESCRIZIONE TASK  -----------------------------*/
/*---------------------------------------------------------------------------*/

/* talkWithSPICAN_Task	: task che comunica con il modulo Pmod CAN.			 */

/* talkWithHost_Task	: task che comunica con il PC host.					 */

/* MsgGenerator_Task	: task che invia messaggi stub nella coda.			 */

/* CircularStudy_Task	: task per verificare il comportamento circolare
 della coda.										 */

/* Print_Task			: task che stampa solo i messaggi dei sensori
 (implementazione obsoleta con la coda vecchia).	 */

/*---------------------------------------------------------------------------*/
/*--------------------------  COME USARE I TASK  ----------------------------*/
/*---------------------------------------------------------------------------*/

/* I task non possono essere usati tutti contemporaneamente. Quindi nel main
 bisogna decommentare i thread da usare e commentare quelli da non usare.	 */

/* Le combinazioni possibili di task da utilizzare sono le seguenti:		 */

/*  - talkWithSPICAN_Task  +  talkWithHost_Task:
 il primo riceve i messaggi dai sensori, il secondo li inoltra all'host.*/

/*  - MsgGenerator_Task  +  talkWithHost_Task:
 il primo genera messaggi stub, il secondo li inoltra all'host. 		 */

/*  - CircularStudy:
 da usare solo a scopo di debug con i breakpoint per verificare il
 comportamento circolare della coda di FreeRTOS.						 */

/*  - talkWithSPICAN_Task  +  Print_Task:
 il primo riceve i messaggi dai sensori, MA SOLO con la coda vecchia,
 il secondo li stampa a video ogni 30 secondi.							 */

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

//Eventualmente, alla fine, se non crea problemi, spostare questi tutti include nella nostra libreria di alto livello.
/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
//#include "timers.h"			//FORSE NON SERVE. Se non serve, alla fine del progetto cancellare.

/* Xilinx includes. */
#include "xil_printf.h"			//implementazione della sola funzione "xil_printf" a scopo di debug (invece di includere la definizione della printf C standard)
#include "xparameters.h"
#include "xuartps.h"			//funzioni di inizializzazione, trasmissione e ricezione verso la UART (verso l'host).
#include "platform.h"			//funzioni init_platform() e cleanup_platform() (CODICE DOTTORANDO)

#include "xtime_l.h"
#include "math.h"

/* Own includes. */
#include "Devicelib.h"			//mettere la nostra libreria (di alto livello) al posto di questa

/* Defines. */
#define TIMER_ID	1					//forse non serve più
#define DELAY_30_SECONDS	30000UL
#define DELAY_10_SECONDS	10000UL
#define DELAY_2_SECONDS		2000UL
#define DELAY_1_SECOND		1000UL
#define DELAY_100_MSECONDS		100UL
#define TIMER_CHECK_THRESHOLD	9		//forse non serve più
#define QUEUE_SIZE 256
#define NUMBER_OF_MSG_TO_TEST 1000 // da 540 le prestazioni cominciano a degradare

/* Thread Functions Definition. */
static void talkWithSPICAN_Task(void *pvParameters);
static void talkWithHost_Task(void *pvParameters);
//static void talkWithHostTest_Task(void *pvParameters);
//static void MsgGenerator_Task(void *pvParameters);
//static void CircularStudy_Task(void *pvParameters);

//SPOSTARE NELLA NOSTRA LIBRERIA DI ALTO LIVELLO
//void RX_canframe_2_0( volatile unsigned int * SPI_Controller, QueueHandle_t queue );

/* Static Variables Definition. */
static TaskHandle_t xTalkWithSPICAN_Task;
static TaskHandle_t xTalkWithHost_Task;
//static TaskHandle_t xTalkWithHostTest_Task;
//static TaskHandle_t xMsgGenerator_Task;
//static TaskHandle_t xCircularStudy_Task;
//static TaskHandle_t xPrint_Task;
static QueueHandle_t xQueue = NULL;
//static QueueHandle_t xQueue_circ = NULL; //coda di test per il thread CircularStudy_Task. alla fine cancellare.

XUartPs_Config *Config_1;
XUartPs Uart_PS_1;

volatile unsigned int * SPI_Controller = (volatile unsigned int *) 0x43c00000; //puntatore alla periferica custom SPI Controller
unsigned int n_msg;	//indica il numero di messaggi CAN presenti nella coda
unsigned int frame_lost = 0;//indica il numero di messaggi CAN che vengono persi quando la coda è piena
unsigned int * n_frame_lost = &frame_lost;

//VARIABILI PER IL TEMPO
uint32_t test_time;

XTime gbl_time_before_test;
XTime *p_gbl_time_before_test = &gbl_time_before_test;

XTime gbl_time_after_test;
XTime *p_gbl_time_after_test = &gbl_time_after_test;

uint32_t cost_time=0;

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

	//CREAZIONE TASK
	//LEGGERE QUESTO COMMENTO PER CAPIRE A CHI DARE MAGGIORE PRIORITA'
	//FORSE BISOGNA DARE PIU' PRIORITA' A "talkWithHostTask". PROVARE

	/* Create the two tasks.  The Tx task is given a lower priority than the
	 Rx task, so the Rx task will leave the Blocked state and pre-empt the Tx
	 task as soon as the Tx task places an item in the queue. */

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

	/*xTaskCreate(talkWithHostTest_Task, (const char *) "talkWithHost_Task",
	 configMINIMAL_STACK_SIZE,
	 NULL,
	 tskIDLE_PRIORITY + 1, &xTalkWithHostTest_Task);
	 */

	/*xTaskCreate(MsgGenerator_Task, (const char *) "MsgGenerator_Task",
	 configMINIMAL_STACK_SIZE,
	 NULL,
	 tskIDLE_PRIORITY, &xMsgGenerator_Task);
	 */

	/*	xTaskCreate(	CircularStudy_Task,
	 ( const char * ) "CS",
	 configMINIMAL_STACK_SIZE,
	 NULL,
	 tskIDLE_PRIORITY + 1,
	 &xCircularStudy_Task);
	 */
	/*	xTaskCreate(	Print_Task,
	 ( const char * ) "pr",
	 configMINIMAL_STACK_SIZE,
	 NULL,
	 tskIDLE_PRIORITY + 1,
	 &xPrint_Task );
	 */

	/* Create the queue used by the tasks.  The Rx task has a higher priority
	 than the Tx task, so will preempt the Tx task and remove values from the
	 queue as soon as the Tx task writes to the queue - therefore the queue can
	 never have more than one item in it. */

	//coda ufficiale
	xQueue = xQueueCreate(QUEUE_SIZE,				//la coda ha 256 elementi
			sizeof(struct CANFrame));//ogni elemento ha dimensione (32 + 8 + 8 = 48 byte)

	//coda di test
	//xQueue_circ = xQueueCreate( 4,							//la coda ha 256 elementi
	//							sizeof( struct CANFrame ) );//ogni elemento ha dimensione (32 + 8 + 8 = 48 byte)

	/* Check the queue was created. */
	configASSERT(xQueue);//questo si può anche togliere visto che non facciamo assert da nessun'altra parte

	//xil_printf("\nConfigurazione main completata...\r\n");

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
/*---------------------------  TASK VERSO ZYBO  -----------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

static void talkWithSPICAN_Task(void *pvParameters) {

	for (;;) {

		RX_canframe_2_0(SPI_Controller, xQueue, n_frame_lost);//FUNZIONE NOSTRA

	}

}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*--------------------------  TASK VERSO HOST  ------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

static void talkWithHost_Task(void *pvParameters) {
	const TickType_t x1second = pdMS_TO_TICKS(DELAY_1_SECOND);
	vTaskDelay(x1second);//ritardo di 1s per far in modo che parta prima il task che parla con la Zybo

	const unsigned char HEARTHBEAT[8] =
			{ 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8 };
	const unsigned char ZERO[8] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

	struct CANFrame * frame_recd;

	init_CANframe(&frame_recd);

	char choice = 0;
//	unsigned char frameBuffer[3840];

	//Allocazione dinamica frameBuffer
	int frameB_size_max = 64;	//limite fisico del buffer della UART della Zybo
	struct CANFrameToSend * frameBuffer =
			(struct CANFrameToSend *) pvPortMalloc(sizeof(frameB_size_max));

	for (;;) {

		XUartPs_Recv(&Uart_PS_1, &choice, 1);

		switch (choice) {
		case '0': //identificazione della porta su cui è connessa la Zybo

			//xil_printf("OK");
			XUartPs_Send(&Uart_PS_1, "OK", 2); //una delle due, vedere quale funziona meglio
			choice = 0;
			break;
		case '1': //restituisce l'hearthbeat

			while (memcmp((unsigned char *) (frame_recd->Data),
					(unsigned char *) HEARTHBEAT, 8)) {
				xQueueReceive(xQueue, (void*) frame_recd, portMAX_DELAY);
			}

			for (int j = 0; j < (frame_recd->DLC); j++) {
				xil_printf("%x", frame_recd->Data[j]);
			}

			choice = 0;
			zero_CANframe(frame_recd);
			break;

		case '2': // restituisce la prima misura di temperatura e pressione disponibile

			while (!memcmp((unsigned char *) (frame_recd->Data),
					(unsigned char *) HEARTHBEAT, 8)
					|| !memcmp((unsigned char *) (frame_recd->Data),
							(unsigned char *) ZERO, 8)
					|| (frame_recd->Data[3] == 0x0 && frame_recd->Data[4] == 0x0)) {

				xQueueReceive(xQueue, (void*) frame_recd, portMAX_DELAY);
			}

			while (XUartPs_IsSending(&Uart_PS_1))
				;
			XUartPs_Send(&Uart_PS_1, (u8 *) frame_recd,
					sizeof(struct CANFrame));
			choice = 0;
			zero_CANframe(frame_recd);
			break;

		case '3': // restituisce il numero di messaggi persi

			while (XUartPs_IsSending(&Uart_PS_1));
			XUartPs_Send(&Uart_PS_1, (u8 *) n_frame_lost, sizeof(n_frame_lost));
			choice = 0;
			break;

		case '4': // restituisce il numero di frame nella cosa

			n_msg = (unsigned int) uxQueueMessagesWaiting(xQueue); //perché const ???
			unsigned int * n_msg_ptr = &n_msg;
			XUartPs_Send(&Uart_PS_1, n_msg_ptr, sizeof(n_msg)); //4 byte
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

			//per ogni messaggio da inviare all'host...
//			int start = (int)clock();

			//start
		    XTime_GetTime(p_gbl_time_before_test);

		    for (int i = 0; i < n_msg; i++) {

				//Estrazione messaggio dalla coda
				xQueueReceive(xQueue, (void*) frame_recd, portMAX_DELAY);

				//Riempimento frameBuffer
				int ofs = i % 4;
				(frameBuffer + ofs)->ID = frame_recd->ID;
				;		//frame_recd->ID;
				(frameBuffer + ofs)->DLC = frame_recd->DLC;	//frame_recd->DLC;
				for (int j = 0; j < (frameBuffer + ofs)->DLC; j++) {
					(frameBuffer + ofs)->Data[j] = frame_recd->Data[j];	//frame_recd->Data[j];
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
						//xil_printf("Tempo iniziale%u\n", (unsigned)time(NULL));
						XUartPs_Send(&Uart_PS_1, (u8 *) frameBuffer,
								n_byte_to_send);
						//xil_printf("Tempo finale%u\n", (unsigned)time(NULL));

					} else {
						n_byte_to_send = n_msg_to_send * 16;
						n_msg_to_send = 0;
						while (XUartPs_IsSending(&Uart_PS_1)) {
						}
						XUartPs_Send(&Uart_PS_1, (u8 *) frameBuffer,
								n_byte_to_send);
					}

					//Azzeramento frameBuffer (per il prossimo invio)
					zero_frameBuffer(frameBuffer);
					cont = 0;
				}

				//Azzeramento frame_recd (per la prossima estrazione)
				zero_CANframe(frame_recd);
			}

		    //STOP
		    XTime_GetTime(p_gbl_time_after_test);

		    test_time = (u64) gbl_time_after_test - (u64) gbl_time_before_test;

		    xil_printf("Tempo di esecuzione =  %d secondi \n", test_time);


		    cost_time = test_time /(uint32_t)((65*1000000)/2) ;
		    xil_printf("Test time = %d sec \n", cost_time);

//			int end = (int)clock();
//			xil_printf("Tempo di esecuzione =  %f secondi \n", ((double)(end - start)) / CLOCKS_PER_SEC);

			n_msg = 0;
			choice = 0;
			break;

		case '6'://invio di NUMBER_OF_MSG_TO_TEST frame per analisi

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
						XUartPs_Send(&Uart_PS_1, (u8 *) frameBuffer,
								n_byte_to_send2);
						usleep(100000);
//						xil_printf("frame inviato %u\n", i);
					} else {
						n_byte_to_send2 = n_msg_to_send2 * 16;
						n_msg_to_send2 = 0;
						while (XUartPs_IsSending(&Uart_PS_1)) {
						}
						XUartPs_Send(&Uart_PS_1, (u8 *) frameBuffer,
								n_byte_to_send2);
//						xil_printf("frame inviato %u\n", i);
					}

					//Azzeramento frameBuffer (per il prossimo invio)
					zero_frameBuffer(frameBuffer);
					cont2 = 0;
				}

				//Azzeramento frame_recd (per la prossima estrazione)
				zero_CANframe(frame_recd);
			}

			choice = 0;
			break;

		default:

			break;

		}

	}

}

//static void talkWithHostTest_Task(void *pvParameters) {
//	const TickType_t x1second = pdMS_TO_TICKS(DELAY_1_SECOND);
//	vTaskDelay(x1second); //ritardo di 1s per far in modo che parta prima il task che parla con la Zybo
//
//	struct CANFrame * frame_recd;
//
//	for (int i = 0;; i++) {
//		vTaskDelay(pdMS_TO_TICKS(100UL));
//
//		init_CANframe(&frame_recd);
//
//		xQueueReceive(xQueue, (void*) frame_recd, portMAX_DELAY);
//
//		xil_printf("%d", frame_recd->ID);
//
//		deinit_CANframe(&frame_recd);
//	}
//
//}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*-----------------------  TASK GENERATORE FITTIZIO  ------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

//static void MsgGenerator_Task(void *pvParameters) {
//
//	struct CANFrame * frame_send;
//
////Inizializzazione singola Frame CAN (messaggi fittizi/stub per fare analisi di reliability)
//	frame_send->ID = 0;						//ID = incrementale a partire da 1
//	frame_send->DLC = 8;							//DLC = 8
//	for (int i = 0; i < frame_send->DLC; i++) {
//
//		frame_send->Data[i] = 0x55;	//Data: ( 0x55 (hex) oppure 'U' (ASCII) ) x8
//	}
//
//	/*	//Stampa frame_send
//	 xil_printf( "\nFrame CAN (stub):\nID = %x\r\n", frame_2->ID );
//	 xil_printf( "DLC = %x\r\n", frame_2->DLC );
//	 xil_printf( "Dato = %x ", frame_2->Data[0] );
//	 for( int j=1; j<(frame_2->DLC)-1; j++ ) {
//	 xil_printf( "%x ", frame_2->Data[j] );
//	 }
//	 xil_printf( "%x\r\n", frame_2->Data[7] );
//	 */
//
////Invio di 256 messaggi stub sulla coda
//	for (int k = 0;; k++) {
//
//		vTaskDelay(pdMS_TO_TICKS(250UL));
//
//		frame_send->ID = k + 1;						//inizializzazione ID
//
//		xQueueSend(xQueue, (void* ) frame_send, 0UL);
//
//		//xil_printf( "scrittura %d \n", k );
//
//	}
//
//	for (;;)
//		;		//non so se serve questo for infinito alla fine di questo task
//
//}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*------------------------  STUDIO CODA CIRCOLARE  --------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

//static void CircularStudy_Task(void *pvParameters) {
//
////ATTENZIONE: la coda usata in questo task ("xQueue_circ") può contenere al massimo 4 elementi.
//
//	struct CANFrame frame[12];
//	struct CANFrame * frame_r;
//
////Generazione array di 12 frame CAN
//	for (int i = 0; i < 12; i++) {
//		frame[i].ID = i + 1;
//		frame[i].DLC = 8;
//		for (int j = 0; j < frame[i].DLC; j++) {
//			frame[i].Data[j] = i + 1;
//		}
//	}
//
////Inizializzazione frame CAN che preleva i frame dalla coda
//	init_CANframe(&frame_r);
//
////Test invio e ricezione (con la funzione base xQueueSend)
//	xQueueSend(xQueue_circ, (void* ) &frame[0], 0UL);
//	xQueueSend(xQueue_circ, (void* ) &frame[1], 0UL);
//	xQueueSend(xQueue_circ, (void* ) &frame[2], 0UL);
//	xQueueSend(xQueue_circ, (void* ) &frame[3], 0UL);
//
//	xQueueSend(xQueue_circ, (void* ) &frame[4], 0UL);
//	xQueueSend(xQueue_circ, (void* ) &frame[5], 0UL);
//
//	xQueueReceive(xQueue_circ, (void*) frame_r, portMAX_DELAY);
//	xQueueReceive(xQueue_circ, (void*) frame_r, portMAX_DELAY);
//
//	xQueueSend(xQueue_circ, (void* ) &frame[5], 0UL);
//	xQueueSend(xQueue_circ, (void* ) &frame[5], 0UL);
//
////Test invio e ricezione (con la funzione CircularQueueSend)
//	/*	CircularQueueSend( xQueue_circ, (void*) &frame[0], 0UL );
//	 CircularQueueSend( xQueue_circ, (void*) &frame[1], 0UL );
//	 CircularQueueSend( xQueue_circ, (void*) &frame[2], 0UL );
//	 CircularQueueSend( xQueue_circ, (void*) &frame[3], 0UL );
//
//	 CircularQueueSend( xQueue_circ, (void*) &frame[4], 0UL );
//	 CircularQueueSend( xQueue_circ, (void*) &frame[5], 0UL );
//
//	 xQueueReceive( xQueue_circ, (void*) frame_r, portMAX_DELAY );
//	 xQueueReceive( xQueue_circ, (void*) frame_r, portMAX_DELAY );
//
//	 CircularQueueSend( xQueue_circ, (void*) &frame[5], 0UL );
//	 CircularQueueSend( xQueue_circ, (void*) &frame[5], 0UL );
//	 */
//
////Stampa degli elementi in coda
//	for (int k = 0; k < 4; k++) {
//
//		xQueueReceive(xQueue_circ, (void*) frame_r, portMAX_DELAY);
//
//		xil_printf("\nFrame CAN (stub):\nID = %x\r\n", frame_r->ID);
//		xil_printf("DLC = %x\r\n", frame_r->DLC);
//		xil_printf("Dato = %x ", frame_r->Data[0]);
//		for (int j = 1; j < (frame_r->DLC) - 1; j++) {
//			xil_printf("%x ", frame_r->Data[j]);
//		}
//		xil_printf("%x\r\n", frame_r->Data[7]);
//	}
//
//// Note sulla gestione della coda circolare:
////  - i nuovi elementi vengono inseriti nella coda "in coda";
////  - se arriva un messaggio quando la coda è piena, tale messaggio viene scartato;
////  - l'elemento estratto viene estratto dalla testa;
//
//// La coda di FreeRTOS non permette all'utente di accedere direttamente ai campi della struttura coda.
//// Quindi non si possono usare ad es. per scorrere gli elementi della coda senza estrarli.
//// Maggiori info qui (leggere il problema e le prime 2 risposte, soprattutto la seconda):
//// https://stackoverflow.com/questions/42490944/dereferencing-pointer-to-incomplete-type-struct
//// Nota: se anche si implementa la soluzione in questo link, comunque accederemmo ai campi della coda
//// in maniera non thread-safe, quindi andrebbe gestito anche questo aspetto.
//
////	for( ;; );		//non so se serve questo for infinito alla fine di questo task
//
//}
