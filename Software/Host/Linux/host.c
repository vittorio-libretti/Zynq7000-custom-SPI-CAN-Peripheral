#include "SerialLink.h"

int main(){

	int serial_port = SerialConnect("/dev/ttyUSB1");
	uint32_t msgbytes = 0;

	if(serial_port){
	
		if(Identify(serial_port)){

			char cmd[6+1];
			
			int choice = 0;

			printf("Identification complete\n\n");

			while(choice != 5){
				
				printf("\t*********SerialLink Linux to Zybo*********\n");
				printf("\t1-How Many Messages? - HMM000\n");
				printf("\t2-How Many Bytes? - HMB000\n");
				printf("\t3-Get Me Bytes - GMBxyz\n");
				printf("\t4-Get Me Messages - GMMxyz\n");
				printf("\t5-Exit\n");
				printf("\n\tCMD: ");
				scanf("%d",&choice);
				getchar();
				switch(choice){

					case 1: //How Many Bytes? - HMB000
						uint32_t msgs;
						msgs = HowManyMessages("HMM000",serial_port);
						printf("\tQueue hosts %03u messages\n\n",msgs);
					break;

					case 2://How Many Bytes? - HMB000
					{	
						uint32_t bytes;
						bytes = HowManyBytes("HMB000",serial_port);
						printf("\tQueue hosts %05u characters, which are %05u bytes!\n\n",bytes,bytes/2);
					}
						
					break;

					case 3://Get Me Bytes? - GMBxyz
					{
						
						unsigned int param = 0;
						char * tempstr;
						char param_str[3];

						printf("\tInsert param: ");						
						scanf("%03u",&param);
						sprintf(param_str,"%03u",param);

						tempstr = (char *)malloc(6*sizeof(char));
						strcpy(tempstr,"GMB");
						printf("\t%s\n",tempstr);
						
						strcat(tempstr,param_str);
						printf("\t%s\n",tempstr);
						msgbytes = GetMeBytes(tempstr,serial_port);
						printf("\tthe first %03u messages hold %05u characters\n\n",param,msgbytes);

						free(tempstr);
					}
					break;

					case 4://Get Me Messages - GMMxyz
					{
						unsigned int param = 0;
						char * tempstr;
						char param_str[3];

						printf("\tInsert param: ");						
						scanf("%03u",&param);
						sprintf(param_str,"%03u",param);

						tempstr = (char *)malloc(6*sizeof(char));
						strcpy(tempstr,"GMM");
						printf("\t%s\n",tempstr);
						
						strcat(tempstr,param_str);
						printf("\t%s\n",tempstr);
						GetMeMessages(tempstr,serial_port,msgbytes);

						free(tempstr);
					}
					break;

					case 5:
						printf("\tHasta la vista...\n");
					break;

					default:
						printf("\tNot valid\n");
					break;
				
				}
				
				
			}
			
		}
		else printf("Device not identified\n");
	}

	SerialDisconnect(serial_port);

	return 0;
}
