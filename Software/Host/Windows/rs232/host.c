/**************************************************

 file: host.c
 purpose: simple demo that receives characters from
 the serial port and print them on the screen,
 exit the program by pressing Ctrl-C

 compile with the command: gcc host.c analysis.c rs232.c -Wall -Wextra -o2 -o host

 **************************************************/

#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "rs232.h"
#include "analysis.h"

void clearArray(unsigned char array[], int size);
void frameParser(unsigned char frameToParse[], unsigned char frameToFilter[], struct CANFrameToReceive **storageCANFrames_tail_ptr_ptr);

int flag;
int *flag_ptr = &flag;

// variabili per l'analisi dei frame
unsigned int msg_valid = 0;
unsigned int msg_not_valid = 0;
unsigned int *cont_valid_msg = &msg_valid;
unsigned int *cont_not_valid_msg = &msg_not_valid;

//storage per messaggi fittizi (caso 6)
struct CANFrameToReceive *storageFrames;		  // puntatore all'elemento iniziale dello storage dell'host
struct CANFrameToReceive *storageFrames_tail_ptr; // puntatore alla prima posizione libera nello storage dell'host

//storage per messaggi dal sistema CAN (caso 5)
struct CANFrameToReceive *storageCANFrames;		  		// puntatore all'elemento iniziale dello storage dell'host
struct CANFrameToReceive *storageCANFrames_tail_ptr;	// puntatore alla prima posizione libera nello storage dell'host

static unsigned int cont_storaged = 0;


int main()
{
	int n = 0, cport_nr = 0, /* /dev/ttyS0 (COM1 on windows) */
		bdrate = 115200;	 /* 9600 baud */

	unsigned char HEARTH_BEAT_FRAME[16] = {0xea, 0x7, 0x0, 0x0, 0x8, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0xdd, 0xee, 0xff};
	unsigned char hearthbeatArray[8] = {0};
	unsigned char numberMsgInQueue[4] = {0};
	unsigned char frameArray[16] = {0};
	unsigned char bufferArray[64] = {0};
	unsigned int NUM_MSG_IN_CODA = 0;
	char mode[] = {'8', 'N', '1', 0};

	// storage lato host

	init_storageFrames(&storageFrames);
	storageFrames_tail_ptr = storageFrames;
	
	init_storageFrames(&storageCANFrames);
	storageCANFrames_tail_ptr = storageCANFrames;

	printf("MAIN : sto per iniziare la funzione OpenComport\n");
	cport_nr = RS232_OpenComport_2_0(cport_nr, bdrate, mode, 0, flag_ptr);

	if (cport_nr == -1)
	{
		printf("Main: Can not open comport: %d\n", cport_nr);
		return (0);
	}
	else
	{ // cport_nr compreso tra 0 e 31
		printf("Main: Comport opened: %d\n", (cport_nr + 1));
	}

	printf("MAIN : sto per iniziare la funzione Identify\n");

	if (flag == 0)
	{
		printf("\tDevice not identified\n");
		return (0);
	}
	printf("\n\n\tIdentification complete\n\n");

	char choice = 0;
	while (1)
	{

		printf("\n\n\t*********RS232 Windows to Zybo*********\n");
		printf("\t1-Is CAN working?\n");
		printf("\t2-First available temperature and pressure\n");
		printf("\t3-Number of lost messages\n");
		printf("\t4-Get the number of messages in queue\n");
		printf("\t5-Get first n frames in queue (select option 4 first)\n");
		printf("\t6-Frame analysis\n");
		printf("\t7-Exit\n");
		printf("\n\t Select option : ");
		scanf("%c", &choice);
		getchar();
		switch (choice)
		{
			int i;
		case '1':
			RS232_SendByte(cport_nr, '1');
			RS232_my_flushTX(cport_nr);
			while (n != 8)
			{
				Sleep(100);
				n = RS232_PollComport(cport_nr, hearthbeatArray, 8);
				RS232_my_flushRX(cport_nr);
			}
			if (n > 0)
			{
				printf("\treceived %i bytes:\n \tHEARTHBEAT: %.8s\n", n, (char *)hearthbeatArray);
			}
			n = 0;
			choice = 0;
			clearArray(hearthbeatArray, 8);
			clearArray(frameArray, 16);
			break;

		case '2':
			RS232_SendByte(cport_nr, '2');
			printf("\tRS232_SendByte...\n");
			RS232_my_flushTX(cport_nr);
			printf("\tRS232_flushTX...\n");
			i = 0;
			while (n == 0)
			{
				if (i % 20 == 0)
					printf("\tin attesa di risposta...\n");
				Sleep(1000);
				n = RS232_PollComport(cport_nr, frameArray, 16);
				printf("\treceived %i bytes...\n", n);
				RS232_my_flushRX(cport_nr);
				i++;
			}
			if (n > 0)
			{
				printf("\treceived sensor frame: ");
				for (long unsigned int i = 0; i < sizeof(frameArray); i++)
				{
					printf("%x ", frameArray[i]);
				}
				printf("\n");

				// parse Id sensor
				unsigned char sensorID[4];
				sensorID[0] = frameArray[7];
				sensorID[1] = frameArray[8];
				sensorID[2] = frameArray[9];
				sensorID[3] = frameArray[10];

				// parse temperature
				int temp = 0;
				unsigned char charTemp[1];
				charTemp[0] = frameArray[11];
				temp = (*((char *)charTemp)) - 52;

				// parse pressure
				int pressure = 0;
				unsigned char charPressure[1];
				charPressure[0] = frameArray[12];
				pressure = (*((char *)charPressure)) * 40;

				// stampa valori
				printf("\tsensor id: ");
				for (long unsigned int i = 0; i < sizeof(sensorID); i++)
				{
					printf("%x ", sensorID[i]);
				}
				printf("\ttemp: %d C\t", temp);
				printf("pressure: %d mBar\r\n", pressure);
			}
			n = 0;
			choice = 0;
			clearArray(hearthbeatArray, 8);
			clearArray(frameArray, 16);
			break;

		case '3':
			RS232_SendByte(cport_nr, '3');
			RS232_my_flushTX(cport_nr); // qua c'è il problema già detto, bisogna reimplementare questa funzione (se ho capito bene)

			while (n == 0)
			{
				Sleep(1000);
				printf("\tin attesa di risposta 1...\n");
				n = RS232_PollComport(cport_nr, numberMsgInQueue, 4); // bisogna usare un buffer che prenda un unsigned int = 4 byte
				printf("\tricevuti %i byte...\n", n);
				RS232_my_flushRX(cport_nr); // QUESTO POTREBBE ESSERE UN PROBLEMA PERCHé POTREBBE FERMARE IL SECONDO MESSAGGIO (bisogna reimplementare)
			}
			if (n > 0)
			{
				printf("\tNumero di messaggi persi: %u\n", *((int *)numberMsgInQueue)); // non so cos'è %ld, dobbiamo stampare un unsigned int = %u
			}
			n = 0;
			clearArray(numberMsgInQueue, 4);
			choice = 0;
			break;

		case '4':
			RS232_SendByte(cport_nr, '4');
			printf("\tRS232_SendByte...\n");
			RS232_my_flushTX(cport_nr);
			printf("\tRS232_flushTX...\n");
			while (n == 0)
			{
				printf("\tin attesa di risposta...\n");
				Sleep(1000);
				n = RS232_PollComport(cport_nr, numberMsgInQueue, 4); // DEVE ARRIVARE UN UNSIGNED INT DI 32 BIT

				RS232_my_flushRX(cport_nr);
			}
			if (n > 0)
			{
				printf("\tbytes of messages number in zybo queue: ");
				printf("%x ", numberMsgInQueue[3]);
				printf("%x ", numberMsgInQueue[2]);
				printf("%x ", numberMsgInQueue[1]);
				printf("%x ", numberMsgInQueue[0]);
				printf("\n");
				// NUM_MSG_IN_CODA = strtoul((char *)buf4, NULL, 16);
				NUM_MSG_IN_CODA = *((int *)numberMsgInQueue);
				printf("\tMessages currently in queue: %u messages.\n", NUM_MSG_IN_CODA); // VERIFICARE CHE LA FORMATTAZIONE SIA CORRETTA
			}
			n = 0;
			clearArray(hearthbeatArray, 8);
			clearArray(numberMsgInQueue, 4);
			choice = 0;
			break;

		case '5':
			printf("\tExtraction of %u messages...\n", NUM_MSG_IN_CODA);
			RS232_SendByte(cport_nr, '5');
			printf("\tRS232_SendByte...\n");
			RS232_my_flushTX(cport_nr);
			printf("\tRS232_flushTX...\n");
			RS232_my_flushRX(cport_nr);
			int count = 0;
			
			int byte_da_ricevere = NUM_MSG_IN_CODA * 16;
			
			while (byte_da_ricevere > 0)
			{

				while (n == 0 && count < 100)
				{
					printf("\n\tin attesa di risposta...");
					Sleep(100);
					n = RS232_PollComport(cport_nr, bufferArray, 64);

					printf("\n\tricevuti %i byte...", n);
					if (n == 0)
						count++;
				}

				if (n > 0)
				{
					byte_da_ricevere = byte_da_ricevere - n;
					printf("\n\treceived %i frame: ", n / 16);
					// for (int i = 0; i < n; i++)
					// {
					// 	printf("%x ", bufferArray[i]);
					// }
				
					unsigned char frameToParse[16] = {0}; ///
					for (int i = 0; i < n / 16; i++)
					{
						copyTwoCANFramesToReceive((struct CANFrameToReceive *)frameToParse, (struct CANFrameToReceive *)bufferArray + i);
						frameParser(frameToParse, HEARTH_BEAT_FRAME, &storageCANFrames_tail_ptr);
					}
				}
				clearArray(bufferArray, 64);
				count = 0;
				n = 0;
			}
			n = 0;
			clearArray(bufferArray, 64);
			clearArray(frameArray, 16);
			clearArray(numberMsgInQueue, 4);
			
			count = 0;
			choice = 0;
			NUM_MSG_IN_CODA = 0;
			RS232_my_flushRX(cport_nr);
			break;

		case '6':
			printf("\tStart integrity test...\n");
			RS232_SendByte(cport_nr, '6');
			printf("\tRS232_SendByte...\n");
			RS232_my_flushTX(cport_nr);
			printf("\tRS232_flushTX...\n");
			RS232_my_flushRX(cport_nr);
			int count2 = 0;
			int byte_da_ricevere2 = NUMBER_OF_MSG_TO_TEST * 16;

			while (byte_da_ricevere2 > 0)
			{
				printf("\n\tbyte_da_ricevere2: %i \n", byte_da_ricevere2);
				while (n == 0 && count2 < 100)
				{
					printf("\tin attesa di risposta...\n");
					Sleep(100);
					n = RS232_PollComport(cport_nr, bufferArray, 64);
                 
				    MySerialStatus(cport_nr);
				 
				 
 					printf("\tricevuti %i byte...\n", n);
					if (n == 0)
						count2++;
				}
				if (count2 == 100)
				{
					byte_da_ricevere2 = 0;
				}
				if (n > 0)
				{
					byte_da_ricevere2 = byte_da_ricevere2 - n;
					printf("\treceived %i frame: ", n / 16);
					for (int i = 0; i < n; i++)
					{
						printf("%x ", bufferArray[i]);
					}
					printf("\r\n");

					// inserisci funzione parse
				bufferArrayAnalysis((unsigned char *)bufferArray, &storageFrames_tail_ptr, cont_valid_msg, cont_not_valid_msg);
					// printf("\tmessaggi validi: %u\t messaggi non validi: %u\n", msg_valid, msg_not_valid);
				}

				clearArray(bufferArray, 64);
				count2 = 0;
				n = 0;
			}
			printf("\n\tmessaggi validi: %u\t messaggi non validi: %u\n", msg_valid, msg_not_valid);
			n = 0;
			clearArray(bufferArray, 64);
			clearArray(frameArray, 16);
			msg_valid = 0;
			msg_not_valid = 0;
			count2 = 0;
			choice = 0;
			RS232_my_flushRX(cport_nr);
			break;

		case '7':
		{
			printf("\tExit...\n");
			return 0;
		}
		break;

		default:
			printf("\tNot valid\n");
			n = 0;
			choice = 0;
			break;
		}
	}

#ifdef _WIN32
	Sleep(100);
#else
	usleep(100000); /* sleep for 100 milliSeconds */
#endif

	return (0);
}

void clearArray(unsigned char array[], int size)
{

	for (int i = 0; i < size; i++)
	{
		array[i] = 0x00;
	}
}

void frameParser(unsigned char frameToParse[], unsigned char frameToFilter[], struct CANFrameToReceive ** storageCANFrames_tail_ptr_ptr)
{
	struct CANFrameToReceive *storageFrame_tail_ptr = (*storageCANFrames_tail_ptr_ptr);
	
	if (memcmp((unsigned char *)(frameToFilter), (unsigned char *)frameToParse, 16) && frameToParse[8] != 0x0 && frameToParse[9] != 0x0)
	{
		// parse Id sensor
		unsigned char sensorID[4];
		sensorID[0] = frameToParse[7];
		sensorID[1] = frameToParse[8];
		sensorID[2] = frameToParse[9];
		sensorID[3] = frameToParse[10];

		// parse temperature
		int temp = 0;
		unsigned char charTemp[1];
		charTemp[0] = frameToParse[11];
		temp = (*((char *)charTemp)) - 52;

		// parse pressure
		int pressure = 0;
		unsigned char charPressure[1];
		charPressure[0] = frameToParse[12];
		pressure = (*((char *)charPressure)) * 40;

		// stampa valori
		printf("\n\tsensor id: ");
		for (long unsigned int i = 0; i < sizeof(sensorID); i++)
		{
			printf("%x ", sensorID[i]);
		}
		printf("\ttemp: %d °C\t", temp);
		printf("pressure: %d mBar\r\n", pressure);
		
		copyTwoCANFramesToReceive(storageFrame_tail_ptr, (struct CANFrameToReceive *)frameToParse);
		(*storageCANFrames_tail_ptr_ptr) = storageFrame_tail_ptr + 1;
		cont_storaged++;
		if( cont_storaged == (NUMBER_OF_MSG_TO_STORE) ) {
			storageCANFrames_tail_ptr = storageCANFrames;
		}
		
	} else {
		printf("\n\tparsing del frame saltato\n");
	}
}
