#pragma once

#ifndef _PLC_H
#define _PLC_H

//#include <sys/types.h>
//
//void StartCodesys(void);
//void StopCodesys(void);
//bool CodeSysIsRunPID(void);
//pid_t GetCodeSysPID(void);

extern bool plcSTOP;
extern bool plcON;
extern int TipoPlc;
extern int TipoBoard;

#define NOBOARD -1
#define BOARD335 0
#define BOARD340 1
#define BOARDCNV 2
#define BOARDPPH 3

#define NOCONF		 0
#define PLCCODESYS   1
#define PLCLOGICLAB  2
#define CNV600		 3
#define PPH600		 4

#define MODALITA_RUN 0
#define MODALITA_COLLAUDO 1
#define MODALITA_UPGRADE 2
#define MODALITA_RUNSD 3

#define RUNMMC		 0
#define RUNSD		 1

bool VerificaSePLCrun(void);
void GestionePLC(void);

void StopRuntimePLC(void);
void StartRuntimePLC(void);
#endif  //_PLC_H
