#include "CustomFilter.h"

void FilterSetup( struct CustomFilter * F, unsigned int PREG0, unsigned int PREG1, unsigned int * TREG, unsigned int APREG, unsigned int CREG, unsigned int TWREG){

	int i;

	//setup the register
	F->Regfile.Parse_register_0 = PREG0;
	F->Regfile.Parse_register_1 = PREG1;
	F->Regfile.AlertPercentage_register = APREG;
	F->Regfile.Combining_register = CREG;
	F->Regfile.TimeWindow_register = TWREG;
	F->Regfile.Status_register = 0x00000000;

	for(i = 0; i < 8; i++) {
		F->Slot[i].Threshold = TREG[i];
		F->Slot[i].Anomalies_Counter = 0;
	}

	F->Alert_Percentage[0] = (F->Regfile.AlertPercentage_register & 0x0000007f);
	F->Alert_Percentage[1] = ((F->Regfile.AlertPercentage_register & 0x00003f80)>>7);
	F->Alert_Percentage[2] = ((F->Regfile.AlertPercentage_register & 0x001fc000)>>14);
	F->Alert_Percentage[3] = ((F->Regfile.AlertPercentage_register & 0x0fe00000)>>21);
	F->Mode = ((F->Regfile.AlertPercentage_register & 0x10000000)>>28);

	F->Global_Counter = 0x00000000;
	F->Time_Window_Counter = F->Regfile.TimeWindow_register;

	//Parsing Mask
	F->ParsedID = (F->Regfile.Parse_register_0 & 0x7ff00000) >> 20;
	F->ParsedDLC = ((F->Regfile.Parse_register_0 & 0x000c0000) >> 18) | ((F->Regfile.Parse_register_1 & 0xC0000000) >> 28);

	F->ParsedDataPosition[0] = (F->Regfile.Parse_register_0 & 0x0000003f);
	F->ParsedDataPosition[1] = ((F->Regfile.Parse_register_0 & 0x00000fc0) >> 6);
	F->ParsedDataPosition[2] = ((F->Regfile.Parse_register_0 & 0x0003f000) >> 12);
	F->ParsedDataPosition[3] = (F->Regfile.Parse_register_1 & 0x0000003f);
	F->ParsedDataPosition[4] = ((F->Regfile.Parse_register_1 & 0x00000fc0) >> 6);
	F->ParsedDataPosition[5] = ((F->Regfile.Parse_register_1 & 0x0003f000) >> 12);
	F->ParsedDataPosition[6] = ((F->Regfile.Parse_register_1 & 0x00fc0000) >> 18);
	F->ParsedDataPosition[7] = ((F->Regfile.Parse_register_1 & 0x3f000000) >> 24);

	printf("PDO Node ID : 0x%03x\nPDO Data Lenght : %d\n",F->ParsedID,F->ParsedDLC);
	for(i = 0; i < F->ParsedDLC; i++){
		printf("DataPosition %d : 0x%02x\n",i,F->ParsedDataPosition[i]);
	}

	F->Regfile.Alert_register = 0x00000000 | ((F->ParsedID & 0x007f) << 24);
	printf("Alert Percentage Register :\n\t1 - 0x%08x\n\t2 - 0x%08x\n\t3 - 0x%08x\n\t4 - 0x%08x\n",F->Alert_Percentage[0],F->Alert_Percentage[1],F->Alert_Percentage[2],F->Alert_Percentage[3]);
	printf("Alert Status Register: 0x%08x\n",F->Regfile.Alert_register);
}

void FilterRead( struct CustomFilter * F, const struct CANFrame C){

	int i;

	if( F->Time_Window_Counter == 0 ){

		int Percentage = 0;
		uint8_t MaxAlertFlag = 0;

		for(i = 0; i < F->ParsedDLC; i++){
			printf("Anomalies : %d\nGlobal: %d\n",F->Slot[i].Anomalies_Counter,F->Global_Counter);
			if( F->Global_Counter != 0) Percentage = 100*F->Slot[i].Anomalies_Counter/F->Global_Counter;
			else Percentage = 0;
			printf("Percentage Slot %d: %d\n",i,Percentage);

			switch(i){

				case 0 :
					if( Percentage >= F->Alert_Percentage[0] && Percentage < F->Alert_Percentage[1])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffffff8) | (0x1);
					else if(Percentage >= F->Alert_Percentage[1] && Percentage < F->Alert_Percentage[2])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffffff8) | (0x2);
					else if(Percentage >= F->Alert_Percentage[2] && Percentage < F->Alert_Percentage[3])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffffff8) | (0x3);
					else if(Percentage >= F->Alert_Percentage[3]){
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffffff8) | (0x4);
						if(F->Mode == 1) MaxAlertFlag = 1 ;
					}

					else
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffffff8) | (0x0);
				break;
				case 1 :
					if( Percentage >= F->Alert_Percentage[0] && Percentage < F->Alert_Percentage[1])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xffffffc7) | ((0x1)<<3);
					else if(Percentage >= F->Alert_Percentage[1] && Percentage < F->Alert_Percentage[2])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xffffffc7) | ((0x2)<<3);
					else if(Percentage >= F->Alert_Percentage[2] && Percentage < F->Alert_Percentage[3])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xffffffc7) | ((0x3)<<3);
					else if(Percentage >= F->Alert_Percentage[3]){
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xffffffc7) | ((0x4)<<3);
						if(F->Mode == 1) MaxAlertFlag = 1 ;
					}

					else
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xffffffc7) | ((0x0)<<3);
				break;
				case 2 :
					if( Percentage >= F->Alert_Percentage[0] && Percentage < F->Alert_Percentage[1])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffffe3f) | ((0x1)<<6);
					else if(Percentage >= F->Alert_Percentage[1] && Percentage < F->Alert_Percentage[2])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffffe3f) | ((0x2)<<6);
					else if(Percentage >= F->Alert_Percentage[2] && Percentage < F->Alert_Percentage[3])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffffe3f) | ((0x3)<<6);
					else if(Percentage >= F->Alert_Percentage[3]){
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffffe3f) | ((0x4)<<6);
						if(F->Mode == 1) MaxAlertFlag = 1 ;
					}

					else
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffffe3f) | ((0x0)<<6);
				break;
				case 3 :
					if( Percentage >= F->Alert_Percentage[0] && Percentage < F->Alert_Percentage[1])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffff1ff) | ((0x1)<<9);
					else if(Percentage >= F->Alert_Percentage[1] && Percentage < F->Alert_Percentage[2])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffff1ff) | ((0x2)<<9);
					else if(Percentage >= F->Alert_Percentage[2] && Percentage < F->Alert_Percentage[3])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffff1ff) | ((0x3)<<9);
					else if(Percentage >= F->Alert_Percentage[3]){
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffff1ff) | ((0x4)<<9);
						if(F->Mode == 1) MaxAlertFlag = 1 ;
					}

					else
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffff1ff) | ((0x0)<<9);
				break;
				case 4 :
					if( Percentage >= F->Alert_Percentage[0] && Percentage < F->Alert_Percentage[1])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xffff8fff) | ((0x1)<<12);
					else if(Percentage >= F->Alert_Percentage[1] && Percentage < F->Alert_Percentage[2])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xffff8fff) | ((0x2)<<12);
					else if(Percentage >= F->Alert_Percentage[2] && Percentage < F->Alert_Percentage[3])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xffff8fff) | ((0x3)<<12);
					else if(Percentage >= F->Alert_Percentage[3]){
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xffff8fff) | ((0x4)<<12);
						if(F->Mode == 1) MaxAlertFlag = 1 ;
					}
					else
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xffff8fff) | ((0x0)<<12);
				break;
				case 5 :
					if( Percentage >= F->Alert_Percentage[0] && Percentage < F->Alert_Percentage[1])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffc7fff) | ((0x1)<<15);
					else if(Percentage >= F->Alert_Percentage[1] && Percentage < F->Alert_Percentage[2])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffc7fff) | ((0x2)<<15);
					else if(Percentage >= F->Alert_Percentage[2] && Percentage < F->Alert_Percentage[3])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffc7fff) | ((0x3)<<15);
					else if(Percentage >= F->Alert_Percentage[3]){
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffc7fff) | ((0x4)<<15);
						if(F->Mode == 1) MaxAlertFlag = 1 ;
					}
					else
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xfffc7fff) | ((0x0)<<15);
				break;
				case 6 :
					if( Percentage >= F->Alert_Percentage[0] && Percentage < F->Alert_Percentage[1])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xffe3ffff) | ((0x1)<<18);
					else if(Percentage >= F->Alert_Percentage[1] && Percentage < F->Alert_Percentage[2])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xffe3ffff) | ((0x2)<<18);
					else if(Percentage >= F->Alert_Percentage[2] && Percentage < F->Alert_Percentage[3])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xffe3ffff) | ((0x3)<<18);
					else if(Percentage >= F->Alert_Percentage[3]){
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xffe3ffff) | ((0x4)<<18);
						if(F->Mode == 1) MaxAlertFlag = 1 ;
					}
					else
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xffe3ffff) | ((0x0)<<18);
				break;
				case 7 :
					if( Percentage >= F->Alert_Percentage[0] && Percentage < F->Alert_Percentage[1])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xff1fffff) | ((0x1)<<21);
					else if(Percentage >= F->Alert_Percentage[1] && Percentage < F->Alert_Percentage[2])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xff1fffff) | ((0x2)<<21);
					else if(Percentage >= F->Alert_Percentage[2] && Percentage < F->Alert_Percentage[3])
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xff1fffff) | ((0x3)<<21);
					else if(Percentage >= F->Alert_Percentage[3]){
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xff1fffff) | ((0x4)<<21);
						if(F->Mode == 1) MaxAlertFlag = 1 ;
					}
					else
						F->Regfile.Alert_register = (F->Regfile.Alert_register & 0xff1fffff) | ((0x0)<<21);
				break;
			}
		}
		printf("Alert Status Register : 0x%08x\n",F->Regfile.Alert_register);
		if( MaxAlertFlag == 1 ) {
			printf("ALERT!!!	\n");
			F->Regfile.Status_register |= 0x00000001;

		}
	}
	else if( F->ParsedID == C.ID ){
		F->Global_Counter++;
		for(i = 0; i < F->ParsedDLC; i++){
			if( C.Data[(F->ParsedDataPosition[i])/8] > F->Slot[i].Threshold ){
				F->Slot[i].Anomalies_Counter++;
			}
		}

	}

}
