
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include "Ptype.h"
#include "candef.h"
#include "Def.h"
#include "Var.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>


int s; 
struct sockaddr_can addr;
struct ifreq ifr;
struct can_frame frame;

int setnonblock(int sock) {
	int flags;
	flags = fcntl(sock, F_GETFL, 0);
	if (-1 == flags)
		return -1;
	return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}


int HW_InitCan (void)
{
	system("ip link set can0 down");
	system("ip link set can0 up type can bitrate 1000000");

	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		//perror("Socket");
		return 1;
	}
	
	strcpy(ifr.ifr_name,"can0" );
	ioctl(s, SIOCGIFINDEX, &ifr);
	
	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		//perror("Bind");
		return 1;
	}
	setnonblock(s); //soket non bloccante
	return 0;	         
}


int HW_ClosetCan (void)
{
	system("ip link set can0 down");
//	if (close(s) < 0) {
//		perror("Close");
//		return 1;
//	}
	return 0;
}	

int HW_SendCan (void)
{	
	if (write(s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
		//perror("Write");
		return 1;
	}
	return 0;
}	

int HW_ReadCan(void)
{
	int nbytes = read(s, &frame, sizeof(struct can_frame));
	if (nbytes < 0) {
		return 1;
	}
	//printf("<--Rxmsg\r\n");
	return 0;
}


void StartTransmission(void)
{
	byte i;

	//	Questa funzione verifica se nei buffer di trasmissione sono stati inseriti dei pacchetti per l'invio.
	//	Nel caso ci sia un pacchetto pronto per l'invio, prova a trasmetterlo e, se ci cono mailbox libere
	//	libera il buffer temporaneo che potr� essere riutilizzato per un nuovo pacchetto, altrimenti lascia tutto
	//	invariato per la prossima esecuzione, in attesa che si liberi qualche mailbox di trasmissione

	for (i = 0; i < N_BUFFER_TX; i++)
	{
		// scorre tutti i buffer di trasmissione interni per cercare un pacchetto in attesa di trasmissione
		if (IntBuffTxCan[ESEGUI_TRASMISSIONE][i] == TRUE)
		{
			//	se pacchetto in attesa di invio carica il buffer del micro e invia il pacchetto

			frame.can_id = ((((unsigned short)(IntBuffTxCan[T_IDR0][i])) << 8) + IntBuffTxCan[T_IDR1][i]) >> 5;
			//CanHandle.pTxMsg->IDE = CAN_ID_STD;
			//CanHandle.pTxMsg->RTR = CAN_RTR_DATA;
			frame.can_dlc  = IntBuffTxCan[T_DLR][i];
			frame.data[0] = IntBuffTxCan[T_DSR0][i];
			frame.data[1] = IntBuffTxCan[T_DSR1][i];
			frame.data[2] = IntBuffTxCan[T_DSR2][i];
			frame.data[3] = IntBuffTxCan[T_DSR3][i];
			frame.data[4] = IntBuffTxCan[T_DSR4][i];
			frame.data[5] = IntBuffTxCan[T_DSR5][i];
			frame.data[6] = IntBuffTxCan[T_DSR6][i];
			frame.data[7] = IntBuffTxCan[T_DSR7][i];

			// Tenta di trasmettere il pacchetto can
			// se la funzione ritorna HAL_BUSY significa che tutte le mail box di trasmissione sono occupate
			// se la funzione ritorna HAL_OK significa che la trasmissione � andata a buon fine

			if (HW_SendCan()== 0)
			{
				//printf("-->Txmsg\r\n");
				//	significa che c'era una mailbox libera per la trasmissione e la trasmissione � andata a buon fine
				IntBuffTxCan[ESEGUI_TRASMISSIONE][i] = FALSE;
				IntBuffTxCan[BUFFER_OCCUPATO][i] = FALSE;
				//TimerAccensioneLedCan = TEMPO_ACCENSIONE_LED_CAN;
			}
		}
	}
}



int RicezioneCanMsg(void)
{
	int nbytes;
	while (true)
	{
		nbytes = read(s, &frame, sizeof(struct can_frame));
		if (nbytes < 0) { return 1; }
		//	alla fine della sua gestione, la funzione HAL_CAN_IRQHandler dopo la ricezione di un frame, chiama questa funzione user
		IntBuffRxCan[R_IDR0] = (frame.can_id << 5) >> 8;
		IntBuffRxCan[R_IDR1] = (frame.can_id << 5) & 0x00FF;

		IntBuffRxCan[R_DSR0] = frame.data[0];
		IntBuffRxCan[R_DSR1] = frame.data[1];
		IntBuffRxCan[R_DSR2] = frame.data[2];
		IntBuffRxCan[R_DSR3] = frame.data[3];
		IntBuffRxCan[R_DSR4] = frame.data[4];
		IntBuffRxCan[R_DSR5] = frame.data[5];
		IntBuffRxCan[R_DSR6] = frame.data[6];
		IntBuffRxCan[R_DSR7] = frame.data[7];
		IntBuffRxCan[R_DLR] =  frame.can_dlc;

		if (ModalitaCan == MASTER)
			GestioneInterruptRxMasterCanOpen();
	}
	return 0;
}