#include "analysis.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

    int n_max_msg_to_memoryze = 1000;
    *frame = (struct CANFrameToReceive *)malloc(sizeof(struct CANFrameToReceive) * n_max_msg_to_memoryze);

    for (int j = 0; j < n_max_msg_to_memoryze; j++)
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

void bufferArrayAnalysis(unsigned char *bufferArray, struct CANFrameToReceive **storageFrames_tail_ptr_ptr, unsigned int *cont_valid_msg, unsigned int *cont_not_valid_msg)
{
    //printf("\n\t**storageFrames_tail_ptr_ptr: %u", (unsigned int) storageFrames_tail_ptr_ptr);
    //printf("\n\t*storageFrames_tail_ptr_ptr: %u", (unsigned int)*storageFrames_tail_ptr_ptr);
    //printf("\n\tInizio analisi buffer");
    struct CANFrameToReceive *analyzeFrame;
    init_CANframeToReceive(&analyzeFrame);
    struct CANFrameToReceive *bufferArray_ptr = (struct CANFrameToReceive *)bufferArray;
    struct CANFrameToReceive *storageFrames_tail_ptr = (*storageFrames_tail_ptr_ptr);

    int stOf = 0; // storageFrames offset

    for (int i = 0; i < 4; i++)
    {

        copyTwoCANFramesToReceive(analyzeFrame, (bufferArray_ptr + i));
        //printf("\n\tInizio confronto frame con HEARTBEAT");
        if (analyzeFrame->ID == 0x7ea &&
            analyzeFrame->DLC == 0x8 &&
            analyzeFrame->Data[0] == 0x01 &&
            analyzeFrame->Data[1] == 0x02 &&
            analyzeFrame->Data[2] == 0x03 &&
            analyzeFrame->Data[3] == 0x04 &&
            analyzeFrame->Data[4] == 0x05 &&
            analyzeFrame->Data[5] == 0x06 &&
            analyzeFrame->Data[6] == 0x07 &&
            analyzeFrame->Data[7] == 0x08 &&
            analyzeFrame->CTRL1 == 0xff &&
            analyzeFrame->CTRL2 == 0xff &&
            analyzeFrame->CTRL3 == 0xff)
        {

            // il messaggio è corretto
            //printf("\n\tInizio copia frame in area storage");
            //printf("\n\tindirizzo storageFrames_tail_ptr + stOf: %u", (unsigned int)storageFrames_tail_ptr + stOf);
            copyTwoCANFramesToReceive((storageFrames_tail_ptr + stOf), analyzeFrame);
            //printf("\n\tFine copia frame in area storage");
            stOf++;
            (*cont_valid_msg)++;
            printf("\n\tMessaggio integro");
        }
        else
        {

            // il messaggio è corrotto! scartare
            (*cont_not_valid_msg)++;
            printf("\n\tMessaggio corrotto!");
            // ATTENZIONE:
            // in questo punto del codice bisognerebbe controllare due cose:
            //  - se i 16 byte sono arrivati tutti ma uno o più di essi sono corrotti (cioè hanno un valore diverso da quello di partenza).
            //	 In questo caso ci basta solo incrementare il contatore, non possiamo fare altro.
            //  - se i si è perso uno o più byte tra questi 16 byte, quindi è possibile riuscire a capire che comunque il messaggio ad un certo punto finisce bene.
            //    In tal caso, bisognerebbe aggiornare il puntatore "bufferArray_ptr" a puntare all'inizio del nuovo messaggio tenendo conto dei byte persi.
        }
        //printf("\n\tFine confronto frame con HEARTBEAT");
    }

    // questo puntatore viene aggiornato a puntare al prossimo spazio disponibile nello storage lato host
    (*storageFrames_tail_ptr_ptr) = storageFrames_tail_ptr + stOf;

    deinit_CANframeToReceive(&analyzeFrame);
    //printf("\n\tFine analisi buffer");
}