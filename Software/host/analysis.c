#include "analysis.h"

void init_CANframeToReceive(struct CANFrameToReceive **frame)
{

    *frame = (struct CANFrameToReceive *)malloc(sizeof(struct CANFrameToReceive));
    (*frame)->ID = 0;
    (*frame)->DLC = 8;
    for (int i = 0; i < (*frame)->DLC; i++)
    {
        (*frame)->Data[i] = 0;
    }
    (*frame)->CTRL1 = 0;
    (*frame)->CTRL2 = 0;
    (*frame)->CTRL3 = 0;
}

void init_storageFrames(struct CANFrameToReceive **frame)
{

    *frame = (struct CANFrameToReceive *)malloc(sizeof(struct CANFrameToReceive) * NUMBER_OF_MSG_TO_STORE);

    for (int j = 0; j < NUMBER_OF_MSG_TO_STORE; j++)
    {

        ((*frame) + j)->ID = j;
        ((*frame) + j)->DLC = 8;
        for (int i = 0; i < ((*frame) + j)->DLC; i++)
        {
            ((*frame) + j)->Data[i] = 0;
        }
        ((*frame) + j)->CTRL1 = 0;
        ((*frame) + j)->CTRL2 = 0;
        ((*frame) + j)->CTRL3 = 0;
    }
}

void deinit_CANframeToReceive(struct CANFrameToReceive **frame)
{
    free(*frame);
}

void deinit_storageFrames(struct CANFrameToReceive **frame)
{
    free(*frame);
}

void copyTwoCANFramesToReceive(struct CANFrameToReceive *frame1, struct CANFrameToReceive *frame2)
{
    //printf("\n\tInizio copia del frame");
    frame1->ID = frame2->ID;
    frame1->DLC = frame2->DLC;
    for (int i = 0; i < frame1->DLC; i++)
    {
        frame1->Data[i] = frame2->Data[i];
    }
    frame1->CTRL1 = frame2->CTRL1;
    frame1->CTRL2 = frame2->CTRL2;
    frame1->CTRL3 = frame2->CTRL3;
    //printf("\n\tFine copia del frame");
}

void bufferArrayAnalysis( unsigned char *bufferArray, struct CANFrameToReceive **storageFrames_tail_ptr_ptr, unsigned int * cont_valid_msg, unsigned int * cont_not_valid_msg )
{

    struct CANFrameToReceive *bufferArray_ptr = (struct CANFrameToReceive *)bufferArray;
    struct CANFrameToReceive *storageFrames_tail_ptr = (*storageFrames_tail_ptr_ptr);

    int stOf = 0;				//storageFrames offset
    unsigned int retry = 0;		//flag per sfasamento nel bufferArray
    unsigned int lost = 0;		//flag per byte perso

    for (int i = 0; i < 4; i++)
    {

    	if( lost == 1 ) {

    		bufferArray_ptr = bufferArray_ptr + 1;
    		bufferArray_ptr = (struct CANFrameToReceive *)(((unsigned char*) bufferArray_ptr) -1);
    	}
    	else {
			if(i !=0){
			//printf("puntatore al mex del buffer prime dell'incremento: %p\n", bufferArray_ptr);
    		bufferArray_ptr =(struct CANFrameToReceive *) bufferArray_ptr + 1;
			//printf("puntatore al mex del buffer dopo l'incremento: %p\n", bufferArray_ptr);
			}
    	}

        if (/*bufferArray_ptr->ID == 0x7ea &&*/
        	bufferArray_ptr->DLC == 0x8 &&
			bufferArray_ptr->Data[0] == 0x01 &&
			bufferArray_ptr->Data[1] == 0x02 &&
			bufferArray_ptr->Data[2] == 0x03 &&
			bufferArray_ptr->Data[3] == 0x04 &&
			bufferArray_ptr->Data[4] == 0x05 &&
			bufferArray_ptr->Data[5] == 0x06 &&
			bufferArray_ptr->Data[6] == 0x07 &&
			bufferArray_ptr->Data[7] == 0x08 &&
			bufferArray_ptr->CTRL1 == 0xdd &&
			bufferArray_ptr->CTRL2 == 0xee &&
			bufferArray_ptr->CTRL3 == 0xff)
        {

            // il messaggio è corretto
            copyTwoCANFramesToReceive((storageFrames_tail_ptr + stOf), bufferArray_ptr);
            stOf++;
            (*cont_valid_msg)++;
			printf("Messaggio corretto");
            lost = 0;
			//printf( "\nFrame CAN:\nID = %x\r\n", bufferArray_ptr->ID );
            //printf( "DLC = %x\r\n", bufferArray_ptr->DLC );
            //printf( "Dato = %x ", bufferArray_ptr->Data[0] );
            //for( int j=1; j<(bufferArray_ptr->DLC)-1; j++ ) {
            // printf( "%x ", bufferArray_ptr->Data[j] );
            //}
            //printf( "%x\r\n", bufferArray_ptr->Data[7] );
            //printf( "CTRL = %x %x %x\r\n", bufferArray_ptr->CTRL1, bufferArray_ptr->CTRL2, bufferArray_ptr->CTRL3 );

        }
        else
        {
			printf("Siamo nello sfasamento\n");
					
			
        	/* -----------------------------------------------------------------------------------------------------------------*/
        	/* ---------------------------------------------  ANALISI SFASAMENTO  ----------------------------------------------*/
        	/* -----------------------------------------------------------------------------------------------------------------*/

        	//Prima di essere sicuri che il messaggio sia corrotto, verifichiamo che i byte arrivati non siano sfasati.
        	//I messaggi possono essere recuperati se sono sfasati al massimo di 3 byte in avanti (grazie ai 3 byte di controllo finali).


        	unsigned char ID_byte1 = *((unsigned char*) bufferArray_ptr);
        	unsigned char ID_byte2 = *(((unsigned char*) bufferArray_ptr)+1);
        	unsigned char ID_byte3 = *(((unsigned char*) bufferArray_ptr)+2);


        	//se il primo byte è 0xff, sposta bufferArray_ptr a puntare 1 byte più avanti
        	if( ID_byte1 == 0xff ) {

        		bufferArray_ptr = (struct CANFrameToReceive *)(((unsigned char*) bufferArray_ptr) +1);
        		retry = 1;
        	}
        	//se il primo byte è 0xee e il secondo byte è 0xff, sposta bufferArray_ptr a puntare 2 byte più avanti
        	else if( ID_byte1 == 0xee && ID_byte2 == 0xff ) {

    			bufferArray_ptr = (struct CANFrameToReceive *)(((unsigned char*) bufferArray_ptr) +2);
    			retry = 1;
        	}
        	//se il primo è 0xdd, il secondo è 0xee e il terzo byte sono 0xff, sposta bufferArray_ptr a puntare 3 byte più avanti
        	else if( ID_byte1 == 0xdd && ID_byte2 == 0xee && ID_byte3 == 0xff ) {

        		bufferArray_ptr = (struct CANFrameToReceive *)(((unsigned char*) bufferArray_ptr) +3);
        		retry = 1;
        	}

        	//se stiamo valutando il 4° messaggio, per evitare problemi di memoria, verifichiamo il messaggio senza byte di controllo (altrimenti il puntatore uscirebbe dal frameBuffer di 64 byte)
            if( retry == 1 && i == 3 ) {

            	if (/* bufferArray_ptr->ID == 0x7ea && */
            		bufferArray_ptr->DLC == 0x8 &&
					bufferArray_ptr->Data[0] == 0x01 &&
					bufferArray_ptr->Data[1] == 0x02 &&
					bufferArray_ptr->Data[2] == 0x03 &&
					bufferArray_ptr->Data[3] == 0x04 &&
					bufferArray_ptr->Data[4] == 0x05 &&
					bufferArray_ptr->Data[5] == 0x06 &&
					bufferArray_ptr->Data[6] == 0x07 &&
					bufferArray_ptr->Data[7] == 0x08)
            	{
            		//il messaggio è sfasato, ma corretto, quindi possiamo memorizzarlo lo stesso

            		//Nota: essendo l'ultimo messaggio, la copia deve essere fatta ad hoc poiché altrimenti il puntatore esce dall'area di memoria consentita
            		(storageFrames_tail_ptr + stOf)->ID = bufferArray_ptr->ID;
            		(storageFrames_tail_ptr + stOf)->DLC = bufferArray_ptr->DLC;
            		for( int i = 0; i < (storageFrames_tail_ptr + stOf)->DLC; i++ ) {
            			(storageFrames_tail_ptr + stOf)->Data[i] = bufferArray_ptr->Data[i];
            		}
            		(storageFrames_tail_ptr + stOf)->CTRL1 = 0xdd;
            		(storageFrames_tail_ptr + stOf)->CTRL2 = 0xee;
            		(storageFrames_tail_ptr + stOf)->CTRL3 = 0xff;

            		stOf++;
                    (*cont_valid_msg)++;
            	}

            }
        	//se stiamo valutando il i primi 3 messaggi, verifichiamo il messaggio intero
            else if ( retry == 1 ) {

            	if(/* bufferArray_ptr->ID == 0x7ea && */
            	   bufferArray_ptr->DLC == 0x8 &&
				   bufferArray_ptr->Data[0] == 0x01 &&
				   bufferArray_ptr->Data[1] == 0x02 &&
				   bufferArray_ptr->Data[2] == 0x03 &&
				   bufferArray_ptr->Data[3] == 0x04 &&
				   bufferArray_ptr->Data[4] == 0x05 &&
				   bufferArray_ptr->Data[5] == 0x06 &&
				   bufferArray_ptr->Data[6] == 0x07 &&
				   bufferArray_ptr->Data[7] == 0x08 &&
				   bufferArray_ptr->CTRL1 == 0xdd &&
				   bufferArray_ptr->CTRL2 == 0xee &&
				   bufferArray_ptr->CTRL3 == 0xff)
            	{
            		//il messaggio è sfasato, ma corretto, quindi possiamo memorizzarlo lo stesso
            		copyTwoCANFramesToReceive((storageFrames_tail_ptr + stOf), bufferArray_ptr);
            		stOf++;
            		(*cont_valid_msg)++;
            	}
            }
			 

        	/* -----------------------------------------------------------------------------------------------------------------*/
        	/* -------------------------------------------  ANALISI PERDITA 1 BYTE  --------------------------------------------*/
        	/* -----------------------------------------------------------------------------------------------------------------*/

            //Se il messaggio è corrotto a causa della perdita di un byte, allora aggiorniamo il puntatore bufferArray_ptr in
            //modo da riuscire a leggere correttamente i restanti messaggi. Questa analisi vale solo per i primi 3 messaggi.


            //se stiamo considerando uno dei primi 3 messaggi del bufferArray
			if( i<3 ) {

				//se si è perso un byte
				if( (bufferArray_ptr->CTRL1 == 0xdd && bufferArray_ptr->CTRL2 == 0xee && bufferArray_ptr->CTRL3 != 0xff) ||		/* si è perso l'ultimo byte */
					(bufferArray_ptr->CTRL1 == 0xdd && bufferArray_ptr->CTRL2 != 0xee && bufferArray_ptr->CTRL3 == 0xff) ||		/* si è perso il penultimo byte */
					(bufferArray_ptr->CTRL1 != 0xdd && bufferArray_ptr->CTRL2 == 0xee && bufferArray_ptr->CTRL3 == 0xff) ) {	/* si è perso un byte tra il primo e il terzultimo */

						//allora bisogna incrementare il contatore (poiché questo messaggio viene scartato lo stesso)
					    (*cont_not_valid_msg)++;
					    lost = 1;
					    //e bisogna aggiornare il puntatore per leggere correttamente il prossimo messaggio all'iterazione successiva del ciclo for principale
					    //(cosa fatta all'inizio del ciclo for grazie al flag "lost")
						printf("Analisi perdita 1 byte");
				}
			}


        	/* -----------------------------------------------------------------------------------------------------------------*/
        	/* ------------------------------------------------  FINE ANALISI  -------------------------------------------------*/
        	/* -----------------------------------------------------------------------------------------------------------------*/

            // Se il messaggio non è sfasato e non si è perso un byte, allora il messaggio è corrotto! bisogna scartarlo ed incrementare il contatore relativo


            if ( retry == 0 && lost == 0 ) {

            	(*cont_not_valid_msg)++;
				printf("messaggio perduto senza analisi");
            }

            retry = 0;
        }

    }

    //questo puntatore viene aggiornato a puntare al prossimo spazio disponibile nello storage lato host
    (*storageFrames_tail_ptr_ptr) = storageFrames_tail_ptr + stOf;
}
