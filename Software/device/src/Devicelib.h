#include "CustomFilter.h"

#include "sleep.h"
#include "xtime_l.h"
#include "xparameters.h"
#include "xil_types.h"
#include "xpseudo_asm.h"
#include "xreg_cortexa9.h"

/* FreeRTOS includes. */			//questi servono perché usiamo le code di freertos anche in questo file
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


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

struct CANFrameToSend{
	unsigned int ID;				/* 32 bit */
	unsigned char DLC;				/*  8 bit */
	unsigned char Data[8];			/*  8 bit */
	unsigned char CTRL1;			/*  8 bit */
	unsigned char CTRL2;			/*  8 bit */
	unsigned char CTRL3;			/*  8 bit */
};


void init_CANframe( struct CANFrame ** frame );

void deinit_CANframe(struct CANFrame ** frame);

void RX_canframe_2_0( volatile unsigned int * SPI_Controller, QueueHandle_t queue, unsigned int* n_lost_frame );

void zero_CANframe( struct CANFrame * frame );

void zero_frameBuffer(struct CANFrameToSend	* frameBuffer);

