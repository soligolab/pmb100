#pragma once

#ifndef _VAR_H
#define _VAR_H

#ifdef VARIABILI_GLOBALI

#define LocInt 		signed short
#define LocWord 	unsigned short
#define LocByte 	unsigned char
#define LocChar 	signed char
#define LocStruct 	struct
#define LocLong 	long
#define LocUnsLong 	unsigned long
#define LocFloat	float
#define LocDouble	double

#else

#define LocInt 		extern signed short
#define LocWord 	extern unsigned short
#define LocByte 	extern unsigned char
#define LocChar 	extern signed char
#define LocStruct 	extern struct 
#define LocLong 	extern long
#define LocUnsLong 	extern unsigned long
#define LocFloat	extern float
#define LocDouble	extern double

#endif

//	PUNTATORE ALLA RAM CONDIVISA CON IL BOOT

LocStruct	RAM_BOOT_CONDIVISA *RamBootCondivisa;

// DEFINIZIONE DEI CAMPI DI BIT

//LocStruct 	FLAGS Flags;
//LocStruct 	FLAGS_IT FlagsIT;
//LocStruct	ERRORI Errore;

//	GESTIONE BASI TEMPI

LocChar		RallentaTimer;
LocWord     IntervalloSecondo;

// VARIABILI PER LA GESTIONE DEI TIMER SOFTWARE

LocWord	 	Time, ToSec, CSec;
LocWord		MaxTime[NUMERO_TIMER], CntTime[NUMERO_TIMER];




//	GESTIONE CAN

LocByte		ModalitaCan;
LocByte		VelocitaBus;
LocByte 	AddressNodoCan;
LocStruct 	ARRAY_IMPIANTO ArrayImpianto;
LocByte		PassiveErrorRx, PassiveErrorTx, BusOffError, OverrunError;
LocByte 	IntBuffRxCan[N_BYTE_INT_RX_CAN];
LocByte 	IntBuffTxCan[N_BYTE_INT_TX_CAN][N_BUFFER_TX];

LocByte 	SdoRxIdent[N_BYTE_SDO_IDENT];
LocByte 	SdoRxCan[N_BYTE_SDO_RX];
LocByte 	ReplySdoRx[N_BYTE_SDO_RX];

LocByte 	StatoCanOpen;
LocByte 	AttesaRispostaSdo;
LocByte 	ContatorePrioritaLow;
LocByte 	ReplyCommandSpecifier;
LocLong 	ReplyDatoSDO;

LocByte		RitardoInitCan;
//LocByte 	TimerAccensioneLedCan;

//	Gestione led can
LocByte 	TimerLampeggioLedCan;
LocByte		TipoAccensioneLedCan, OldTipoAccensioneLedCan, ConteggioBlinkLedCan, StatoLedCan;
LocUnsLong	MillisecondiLedSyncro;


#endif //_VAR_H