
#include "SPI.h"

struct CustomFilter{

	unsigned short ParsedID;
	unsigned short ParsedDLC;
	unsigned char ParsedDataPosition[8];
	unsigned int Time_Window_Counter;
	unsigned short Global_Counter;
	unsigned char Alert_Percentage[4];
	unsigned char Mode;

	struct FilterSlot{
		unsigned short Anomalies_Counter;
		unsigned int Threshold;
	} Slot[8];

	struct Registers{

		unsigned int Parse_register_0;
		unsigned int Parse_register_1;
		unsigned int Alert_register;
		unsigned int Combining_register;
		unsigned int TimeWindow_register;
		unsigned int AlertPercentage_register;
		unsigned int Status_register;

	} Regfile;

};

void FilterSetup( struct CustomFilter * F, unsigned int PREG0, unsigned int PREG1, unsigned int * TREG, unsigned int AREG, unsigned int CREG, unsigned int TWREG);
void FilterRead( struct CustomFilter *, const struct CANFrame );
