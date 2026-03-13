#pragma once

#ifndef _DEF_H
#define _DEF_H

//#define PLE500INDEX


/***************************************************************************************************************************/
/*									DEFINIZIONE DEL NUMERO DELLA VERSIONE SOFTWARE										   */
/***************************************************************************************************************************/

#define VERSIONE_SOFTWARE						105		//	N.B.: 	Aggiornare anche versione firmware programma
														//			nel file "startup_stm32.s"

/***************************************************************************************************************************/
/*									DEFINIZIONE DELLO SWITCH DI COMPILAZIONE 											   */
/***************************************************************************************************************************/

#define VERSIONE_PL500							1
#define VERSIONE_PL600_PL700					2

#define VERSIONE_COMPILAZIONE					VERSIONE_PL500

/***************************************************************************************************************************/
/*									DEFINIZIONE DELL'ID DEL DISPOSITIVO													   */
/***************************************************************************************************************************/

#define PL500_PL600_PL700_335_1AD				460

#define ID_DISPOSITIVO							PL500_PL600_PL700_335_1AD

/***************************************************************************************************************************/
/*											DEFINIZIONE DEI TIPI DI VARIABILI											   */
/***************************************************************************************************************************/

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long Dword;

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

typedef unsigned char boolean;
typedef unsigned char BOOLEAN;


/***************************************************************************************************************************/
/*											DEFINIZIONE DELLE COSTANTI GENERICHE										   */
/***************************************************************************************************************************/

typedef enum
{
	FALSE = 0,
	TRUE  = !FALSE
}
Bool;


typedef enum
{
	OFF = 0,
	ON  = 1
}
OffOn;

typedef enum
{
	NO  = 0,
	YES = 1
}
NoYes;

typedef enum
{
	PARZIALE = 0,
	TOTALE   = 1
}
ParzialeTotale;

typedef enum
{ DISABILITATO = 0,
	DA_SERIALE   = 1,
	DA_CAN       = 2
}
TipoBootUpgrade;


typedef enum
{ NO_ACTIVE_INTERFACE = 0,
	COM_ACTIVE_USART1   = 1,
	COM_ACTIVE_USART2   = 2,
	COM_ACTIVE_USART3   = 3,
	COM_ACTIVE_CAN      = 4
}
ComActiveInterface;

typedef enum
{
	OPEN  = 32767,
	SHORT = -32768,
}
ErroreSonda;

typedef enum
{
	MASTER = 0,
	SLAVE  = 1
}
ModoCan;

#define OK						0x00


/***************************************************************************************************************************/
/*											DEFINIZIONE DEI TIMER SOFTWARE												   */
/***************************************************************************************************************************/

#define CLCSEC									0x80
#define UN_DECIMO								10				// 	conteggi per ogni decimo di secondo
#define NUMERO_TIMER 							10				// 	numero massimo dei timer che si vogliono utilizzare

#define TIMER_RITARDO_AVVIO_MASTER_CAN			0
#define TEMPO_RITARDO_AVVIO_MASTER_CAN			5*UN_DECIMO

#define TIMER_OFFLINE_SPI						1
#define TEMPO_OFFLINE_INIZIALE_SPI				300*UN_DECIMO
#define TEMPO_OFFLINE_SPI						80*UN_DECIMO
#define TEMPO_AGGIORNAMENTO_BUFFER_SPI			75*UN_DECIMO

#define TIMER_MAX_ATTESA_BUFFER_LIBERO			2
#define TEMPO_MAX_ATTESA_BUFFER_LIBERO			15*UN_DECIMO



/************************************************************************************************/
/*							DEFINIZIONI COSTANTI PER LA GESTIONE DEL PLC						*/
/************************************************************************************************/


#define NUMERO_NODI_MASTER_RETE_PLC					1
#define NUMERO_NODI_SLAVE_RETE_PLC					126

#define NUMERO_NODI_RETE_PLC						NUMERO_NODI_MASTER_RETE_PLC + NUMERO_NODI_SLAVE_RETE_PLC

#define INDICE_NODO_MASTER							0
#define INDICE_NODO_SLAVE_1							1
#define INDICE_NODO_SLAVE_126						126

#define INDICE_PRIMO_NODO_SLAVE_RETE_PLC			INDICE_NODO_SLAVE_1
#define INDICE_ULTIMO_NODO_SLAVE_RETE_PLC			INDICE_NODO_SLAVE_126

#define ADDRESS_PRIMO_NODO_SLAVE_RETE_PLC			INDICE_PRIMO_NODO_SLAVE_RETE_PLC
#define ADDRESS_ULTIMO_NODO_SLAVE_RETE_PLC			INDICE_ULTIMO_NODO_SLAVE_RETE_PLC
#define ADDRESS_NODO_MASTER							0x7F

/***************************************************************************************************************************/
/*					Definizione delle costanti per la gestione dei moduli di I/O Pixsys tramite CANbus					   */
/***************************************************************************************************************************/

//	Gestione procedura di numerazione

//	Valori di ArrayImpianto.StepNumerazioneRete
#define START_NUMERAZIONE							0
#define SET_ADDRESS_NODO_1							1
#define ATTESA_NUMERAZIONE_NODO_1					2
#define SET_MICRO_ID_OUT_NODO_1						3
#define ATTESA_SET_MICRO_ID_OUT_NODO_1				4
//	...
#define SET_ADDRESS_NODO_126						501
#define ATTESA_NUMERAZIONE_NODO_126					502
#define SET_MICRO_ID_OUT_NODO_126					503
#define ATTESA_SET_MICRO_ID_OUT_NODO_126			504

#define INVIO_RESET_USCITA_NUMERAZIONE				0xFFFD
#define NUMERAZIONE_FALLITA							0xFFFE
#define NUMERAZIONE_COMPLETATA						0xFFFF

#define NUMERO_INVII_CODICE_START_NUMERAZIONE		6
#define MAX_NUMERO_TENTATIVI_NUMERAZIONE_NODO		6
#define MAX_NUMERO_TENTATIVI_ATTIVAZIONE_ID_OUT		6

//	Ritardi per gestione procedura di numerazione
#define RITARDO_COMANDO_INIZIA_NUMERAZIONE			10		//	attende per max 10 * 1ms = 10ms
#define RITARDO_STABILIZZA_USCITA_NUMERAZIONE		15		//	attende per max 15 * 1ms = 15ms
#define RITARDO_ATTESA_RISPOSTA_NUMERAZIONE_NODO	10		//	attende per max 10 * 1ms = 10ms
#define RITARDO_COMANDO_ATTIVAZIONE_USCITA			2		//	attende per max 2 * 1ms = 2ms
#define RITARDO_INVIO_RESET_FINE_NUMERAZIONE		200  	//	attende per max 200 * 1ms = 200ms


#endif //_DEF_H