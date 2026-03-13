#pragma once

#ifndef _TERMINALELED_H
#define _TERMINALELED_H


/***************************************************************************************************************************/
/*						Definizione delle costanti per la gestione dei led del terminale							 	   */
/***************************************************************************************************************************/
#define LED_RUN										0x1
#define LED_EXP										0x2
#define LED_CAN1									0x4
#define LED_COM1									0x8
#define LED_COM2									0x10
#define LED_VPN										0x20
#define LED_ETH0									0x40
#define LED_ETH1									0x80
#define LED_WIFI									0x100

#define LED_INTERNO									0x1
#define LED_STOP									0x2
#define LED_E_MMC									0x4
#define LED_E_EXP									0x8
#define LED_E_CAN1									0x10
#define LED_E_1										0x20
#define LED_E_2										0x40
#define LED_W_1										0x80
#define LED_DISCO									0x100


//#define LED_A_COMANDO_LOCALE						0x40000
//#define SET_LED_DHCP								0x80000
//#define RESET_LED_DHCP							0x100000
//#define RESET_FLAG_PULSANTE_POWER_ON				0x200000

/***************************************************************************************************************************/
/*						Definizione delle costanti per la gestione dei dip-switch del terminale							   */
/***************************************************************************************************************************/

#define BIT_DIP1_STOP								0x01
#define BIT_DIP2_192_168_0_99						0x02
#define BIT_DIP3_NFC								0x04
#define BIT_DIP4_VPN								0x08
#define BIT_PULSANTE								0x10
#define BIT_PULSANTE_PON							0x20
#define BIT_NFC_ACTIVITY							0x40
#define BIT_NFC_MODIFY								0x80


//using namespace std;
extern unsigned short		LedTRMsup, LedTRMinf, LedBLKsup, LedBLKinf;
extern unsigned char		Dip, Rotary;
//extern bool					TerminaleOffline;
extern bool					TerminaleOnlineAtStart;
extern bool					blampallled;
extern bool					blampallledinf;
extern bool					blampallledsup;
extern bool					bPrimaLetturaTRM;
extern bool					PlPowerOff;
extern int					RunDisk;
extern int					LifeTimeMMC_1to10;
extern int msleep(long msec);
extern bool					ReadParNFC;
extern bool					ReadParNFCok;


void InizializzaTerminaleLedUart(bool stato);
void InizializzaTerminaleLedSpi(void);
int ReadSerialeLed(void);
void StopDebug_tty0(void);
void StartDebug_tty0(void);
void ApplicaParametriNFC();
unsigned long HAL_GetTick(void);
void HAL_IncTick(void);
void TimerStopLampeggioAllled(bool starttimer);
void TimerStopLampeggioAllledInf(bool starttimer);

#endif

