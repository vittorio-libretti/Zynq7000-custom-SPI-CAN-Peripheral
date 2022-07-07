#include "CustomFilter.h"

#include "sleep.h"
#include "xtime_l.h"
#include "xparameters.h"
#include "xil_types.h"
#include "xpseudo_asm.h"
#include "xreg_cortexa9.h"

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


#define QMAXSIZE 256
#define NUM_OF_BYTE 6
#define IDLE 0
#define IDENTIFIED 1
#define HOSTCOM 2
#define CANCOM 3

struct BufferQueue{
	uint8_t size;
	uint32_t bytes;
	uint8_t head;
	uint8_t tail;
	struct CANFrame frame[256];
};

void init_queue(struct BufferQueue **);
void deinit_queue(struct BufferQueue **);
u8 get_msg_bytes(struct BufferQueue *,u8 payload);
void get_msg(struct BufferQueue *, u8, char *);
void RX_canframe(volatile unsigned int * SPI_Controller, struct BufferQueue * Q);
void FreeHead(struct BufferQueue *);

void cleanmsg(char *);
int usleep_A9(uint32_t useconds);
