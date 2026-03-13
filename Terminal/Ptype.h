#pragma once

#ifndef _PTYPE_H
#define _PTYPE_H

#include "Def.h"
#include <sys/types.h>

/***************************************************************************************************************************/
/*								SEZIONE DELLE DEFINIZIONE DELLE STRUTTURE DI DATI DEL PROGRAMMA							   */
/***************************************************************************************************************************/

struct ARRAY_IMPIANTO
{
	unsigned short 	StepNumerazioneRete;
	unsigned short	TimerNumerazione;
	unsigned char 	TentativiNumerazione;
	unsigned char 	NumeroNodiRilevati;
	unsigned char 	SlaveAddressInNumerazione;
	unsigned char 	AttesaRispostaNumerazione;
	unsigned char	AttesaRispostaAttivazioneUscita;
};

/***************************************************************************************************************************/
/*									   DEFINIZIONE DEI PROTOTIPI DI FUNZIONE											   */
/***************************************************************************************************************************/

//	Util.c
int msleep(long msec);
pid_t GetPID(char * filename);
bool PIDisRun(pid_t pid);

// PLC.c




//	can_master.c
void InizializzaMasteCAN(byte BusSpeed);
void SettaFiltroIdentificatoreCAN1(byte Filtro);
void InizializzaReteCanOpen(void);
char SequenzaNumerazione(void);
void TrasmettiComandoNumerazione(unsigned char Code, unsigned char Slave);
void GestioneTrasmissioneMasterCanOpen(void);
char BufferTrasmissioneLibero(void);
void TrasmettiComandoNMT(unsigned char CommandSpecifier, unsigned char NSlave);
void GestioneInterruptRxMasterCanOpen(void);



//	timer.c
void GestioneTimer(void);
void InizializzaTimer(void);
void TimerOn(unsigned short nto, unsigned short wait_t);
void TimerStop(unsigned short nto);
void ClearTimer(unsigned short nto);
unsigned char TimerOut(unsigned short nto);
unsigned short QuantoTempoManca(unsigned short nto);
unsigned short BaseTempi(void);


//	main.c


// CanHW.c

int HW_InitCan(void);
int HW_ClosetCan(void);
int HW_SendCan(void);
int HW_ReadCan(void);
void StartTransmission(void);
int RicezioneCanMsg(void);

// can_master


// ServizioDeviceFinder.c
void StartFinder(void);
void TestGestioneFinder(void);

// TerminaleLed.c



#endif  //_PTYPE_H
