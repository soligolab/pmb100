#pragma once

#ifndef _CANDEF_H
#define _CANDEF_H

int GestioneNumerazioneNodiCan(void);
extern bool PLE500INDEX;

/***************************************************************************************************************************/
/*													GESTIONE CANBUS														   */
/***************************************************************************************************************************/
#define INIT_CAN							1

/***************************************************************************************************************************/
/*												GESTIONE CANOPEN														   */
/***************************************************************************************************************************/
//	Buffer di ricezione
#define N_BYTE_INT_RX_CAN					11

#define R_IDR0								0
#define R_IDR1								1
#define R_DSR0								2
#define R_DSR1								3
#define R_DSR2								4
#define R_DSR3								5
#define R_DSR4								6
#define R_DSR5								7
#define R_DSR6								8
#define R_DSR7								9
#define R_DLR								10

//	Buffer di trasmissione
#define N_BYTE_INT_TX_CAN					14
#define N_BUFFER_TX							1	//2	alle basse velocit�, non gestendo la priorit�, risciava di trasmettere sempre il buffer 1 e lasciare sempre in coda il buffer 2

#define T_IDR0								0
#define T_IDR1								1
#define T_DSR0								2
#define T_DSR1								3
#define T_DSR2								4
#define T_DSR3								5
#define T_DSR4								6
#define T_DSR5								7
#define T_DSR6								8
#define T_DSR7								9
#define T_DLR								10
#define T_TBPR								11
#define BUFFER_OCCUPATO						12
#define ESEGUI_TRASMISSIONE					13

//	Codici oggetti CanOpen
#define	NMT									0x00	//	0x0000
#define NMT_ERROR_CTRL						0xE0	//	0x0700

//	Codici "Command Specifier" dell'NMT
#define RESET_NODE							0x81
#define COMANDO_NUMERAZIONE_NODO			0x83
#define COMANDO_DISATTIVA_MICRO_ID_OUT		0x84

// Codici stato "Node Guarding" o "HeartBeat"
#define STOPPED								0x04

/***************************************************************************************************************************/
/*												GESTIONE SDO CANOPEN													   */
/***************************************************************************************************************************/

#define N_BYTE_SDO_IDENT					2
#define N_BYTE_SDO_RX						8
#define TEMPO_BOOT_UP_DEFAULT				20			//	20 centesimi di secondo = 20 * 0.01 = 200 ms

#endif // _CANDEF_H
