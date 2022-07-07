/**************************************************

 file: demo_rx.c
 purpose: simple demo that receives characters from
 the serial port and print them on the screen,
 exit the program by pressing Ctrl-C

 compile with the command: gcc demo_rx.c rs232.c -Wall -Wextra -o2 -o test_rx

 **************************************************/

#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "rs232.h"

void clearArray(unsigned char array[]);

int main()
{
	int n = 0, cport_nr = 7, /* /dev/ttyS0 (COM1 on windows) */
		bdrate = 115200;	 /* 9600 baud */

	const unsigned char HEARTH_BEAT_FRAME[16] = {0xea, 0x7, 0x0, 0x0, 0x8, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8};
	;
	unsigned char hearthbeatArray[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	unsigned char numberMsgInQueue[4] = {0, 0, 0, 0};
	unsigned char frameArray[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	unsigned int NUM_MSG_IN_CODA = 0;
	char mode[] = {'8', 'N', '1', 0};

	if (RS232_OpenComport(cport_nr, bdrate, mode, 0))
	{
		printf("Can not open comport\n");

		return (0);
	}

	printf("\n\n\tIdentification complete\n\n");
	char choice = 0;
	while (choice != 4)
	{

		printf("\n\n\t*********RS232 Windows to Zybo*********\n");
		printf("\t1-Is CAN working?\n");
		printf("\t2-Fist available temperature and pressure\n");
		printf("\t3-Number of lost messages\n");
		printf("\t4-Get the number of messages in queue\n");
		printf("\t5-Get first n frames in queue (select option 5 first)\n");
		printf("\t6-Exit\n");
		printf("\n\tCMD: ");
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
			clearArray(hearthbeatArray);
			clearArray(frameArray);
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
				printf("\ttemp: %d °C\t", temp);
				printf("pressure: %d mBar\r\n", pressure);
			}
			n = 0;
			choice = 0;
			clearArray(hearthbeatArray);
			clearArray(frameArray);
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
			clearArray(numberMsgInQueue);
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
			clearArray(hearthbeatArray);
			clearArray(numberMsgInQueue);
			choice = 0;
			break;

		case '5':
			printf("\tExtraction of %u messages...\n", NUM_MSG_IN_CODA);
			RS232_SendByte(cport_nr, '5');
			printf("\tRS232_SendByte...\n");
			RS232_my_flushTX(cport_nr);
			printf("\tRS232_flushTX...\n");
			int count = 0;
			for (unsigned int i = 0; i < NUM_MSG_IN_CODA; i++)
			{
				while (n == 0 && count < 10)
				{
					printf("\tin attesa di risposta...\n");
					Sleep(50);
					n = RS232_PollComport(cport_nr, frameArray, 16);

					if (n == 0)
						count++;
				}

				if (n > 0)
				{
					printf("\treceived %i frame: ", i + 1);
					for (long unsigned int i = 0; i < sizeof(frameArray) - 3; i++)
					{
						printf("%x ", frameArray[i]);
					}
					printf("\n");
				}
				if (memcmp((unsigned char *)(HEARTH_BEAT_FRAME), (unsigned char *)frameArray, 16) && frameArray[8] != 0x0 && frameArray[9] != 0x0)
				{
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
					printf("\ttemp: %d °C\t", temp);
					printf("pressure: %d mBar\r\n", pressure);
				}
				n = 0;
				clearArray(frameArray);
			}
			count = 0;
			choice = 0;
			NUM_MSG_IN_CODA = 0;
			break;

		case '6':
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

void clearArray(unsigned char *array)
{
	for (long unsigned int i = 0; i < sizeof(array); i++)
	{
		array[i] = 0;
	}
}
