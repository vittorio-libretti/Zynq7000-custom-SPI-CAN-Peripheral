#include "CustomFilter.h"

#include "sleep.h"
#include "xtime_l.h"
#include "xparameters.h"
#include "xil_types.h"
#include "xpseudo_asm.h"
#include "xreg_cortexa9.h"

#include <stdbool.h>				//Forse non serve più. Serviva per la funzione "CircularQueueSend()"


/* FreeRTOS includes. */			//questi servono perché usiamo le code di freertos anche in questo file
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
//#include "timers.h"				//FORSE NON SERVE. Se non serve, alla fine del progetto cancellare.



#if defined (SLEEP_TIMER_BASEADDR)
#include "xil_sleeptimer.h"
#endif

/****************************  Constant Definitions  ************************/
#if defined (SLEEP_TIMER_BASEADDR)
#define COUNTS_PER_USECOND (SLEEP_TIMER_FREQUENCY / 1000000)
#else
/* Global Timer is always clocked at half of the CPU frequency */
#define COUNTS_PER_USECOND  (XPAR_CPU_CORTEXA9_CORE_CLOCK_FREQ_HZ / (2U*1000000U))
#endif


#define QMAXSIZE 256				//questi poi bisogna capire quali servono e quali no
#define NUM_OF_BYTE 6
#define IDLE 0
#define IDENTIFIED 1
#define HOSTCOM 2
#define CANCOM 3


struct BufferQueue{					//coda vecchia del dottorando. alla fine non servirà più
	uint8_t size;
	uint32_t bytes;
	uint8_t head;
	uint8_t tail;
	struct CANFrame frame[256];
};


/* ------- FUNZIONI DEL DOTTORANDO ------- */

void init_queue(struct BufferQueue **);
void deinit_queue(struct BufferQueue **);
u8 get_msg_bytes(struct BufferQueue *,u8 payload);
void get_msg(struct BufferQueue *, u8, char *);
void RX_canframe(volatile unsigned int * SPI_Controller, struct BufferQueue * Q);
void FreeHead(struct BufferQueue *);

void cleanmsg(char *);
int usleep_A9(uint32_t useconds);


/* ------- FUNZIONI NOSTRE ------- */

void CircularQueueSend(QueueHandle_t queue, struct CANFrame * frame, TickType_t blocktime);		//forse non serve più

void init_CANframe( struct CANFrame ** frame );

void deinit_CANframe(struct CANFrame ** frame);

void RX_canframe_2_0( volatile unsigned int * SPI_Controller, QueueHandle_t queue, unsigned int* n_lost_frame );

void zero_CANframe( struct CANFrame * frame );



