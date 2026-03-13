#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <util.h>
#include "terminale_led.h"
#include "plc.h"

int TimerGestLedEth;
int TimerGestErrLedEth;
int Timercan0errdet=2+1;
int Timercan1errdet;

int TimerGetdiskstats;
int TimerByteSecond;

int OldPackcan0=-1;
int OldPackcan1=-1;
bool fcan0open;
bool fcan1open;
bool fcan0error;
bool fcan1error;


int oldsectorwr_MMC;
int oldsectorwr_FRAM;
int bytesecondDISK_wr;
int bytesecondFRAM_wr;

char buffer_diskstats[1024];
bool GestioneErroriCan = true;
bool RdFileGestioneErroriCan = true;

int InfoDiskAndLed(char *diskname, unsigned int led, int * oldsectorwr )
{
	int sectorwr;
	char *token;
	int  lendiskname = strlen(diskname);
	int  npar_diskstats = 10; //sector writen
	const char s[2] = " ";	  //separatore dati
	int n = 3;
	token = strtok(buffer_diskstats, s);
	//cerca il disco
	while (token != NULL)
	{	
		int r = strncmp(token, diskname, lendiskname);
		if (r == 0)
		{
			break;
		}
		/* get the next token */
		token = strtok(NULL, s);
	}
	token = strtok(NULL, s);
	//cerca il parametro
	while (token != NULL)
	{
		n++;
		if (n == npar_diskstats)	break;
		/* get the next token */
		token = strtok(NULL, s);
	}
	sectorwr = atoi(token);
	if (sectorwr != *oldsectorwr)
	{
		*oldsectorwr = sectorwr;
		LedTRMinf |= led; //on led 
	}
	else
	{
		LedTRMinf &= ~led; //off led 
	}
	return sectorwr; 
}


int CalcoloBytesMinute_Wr(unsigned int led, int sectorwr)
{
	int byteMinute_wr;
	//calcolo byte/secondo in scrittura
	float uptime; // tempo di POWER_ON in secondi
	//FILE* proc_uptime_file = fopen("/proc/uptime", "r");
	//fscanf(proc_uptime_file, "%f", &uptime);
	//fclose(proc_uptime_file);
	
	uptime = 59;
	byteMinute_wr = (sectorwr * 512) / uptime;
	if (byteMinute_wr > 5000)
	{
		//LedBLKinf |= led; //on blink led
		LedTRMinf |= led; //on led 
	}
	else
	{
		//LedBLKinf &= ~led; //off blink led 
	}
	return byteMinute_wr;
}

int sectorwrDISKold=0;
int sectorwrFRAMold=0;
void GestioneledDISK()
{
	int sectorwrDISK;
//	int sectorwrFRAM;
	TimerGetdiskstats++;
	if (TimerGetdiskstats >= 10)//10
	{
		TimerGetdiskstats = 0;
		
		//Led scrittura su Disco di boot mmc o sd
		int fdiskstats = open("/proc/diskstats", O_RDONLY);
		read(fdiskstats, &buffer_diskstats, sizeof(buffer_diskstats));
		close(fdiskstats);
		if (RunDisk==RUNSD)
		{
			sectorwrDISK = InfoDiskAndLed((char*)"mmcblk0", LED_DISCO, &oldsectorwr_MMC); //SD
		}
		else
		{
			sectorwrDISK = InfoDiskAndLed((char*)"mmcblk1", LED_DISCO, &oldsectorwr_MMC); //MMC
		}
		
		//led scrittura su MRAM
//		fdiskstats = open("/proc/diskstats", O_RDONLY);
//		read(fdiskstats, &buffer_diskstats, sizeof(buffer_diskstats));
//		close(fdiskstats);
//		sectorwrFRAM  = InfoDiskAndLed((char*)"mtdblock0", LED_W_1, &oldsectorwr_FRAM); //led E.Batt
		
		//led  mmc usurata
		if (LifeTimeMMC_1to10 >= 9)
		{
			LedTRMinf |= LED_E_MMC;
			LedBLKinf |= LED_E_MMC;
		}
		
		//scrittura bytes minuto
		if (TimerByteSecond >= 59)
		{
			TimerByteSecond = 0;
			bytesecondDISK_wr  = CalcoloBytesMinute_Wr(LED_DISCO, sectorwrDISK - sectorwrDISKold);
//			bytesecondFRAM_wr = CalcoloBytesMinute_Wr(LED_W_1, sectorwrFRAM - sectorwrFRAMold);
			sectorwrDISKold = sectorwrDISK;
//			sectorwrFRAMold = sectorwrFRAM;
		}
		else
			TimerByteSecond++;
			
	}
}
	
//char *diskname = (char*)"mmcblk0"; //"mtdblock0"_9; // "mmcblk0"_7;

void GestioneledNET(void)
{
	int fdled;
	TimerGestLedEth++;
	TimerGestErrLedEth++;
	
	if (TimerGestLedEth >= 10) //entra ogni secondo 
	{
		//led eth1
		fdled = open("/sys/class/net/eth0/carrier", O_RDONLY);	
		if (fdled == -1)
		{
			LedTRMsup &= ~LED_ETH0;//0x00020; //off led eth1
		}
		else
		{
			char buffer;
			read(fdled, &buffer, sizeof(buffer));
			if (buffer == '1') 
				LedTRMsup |= LED_ETH0;//0x00020; //on led eth1
			else
				LedTRMsup &= ~LED_ETH0; //0x00020; //off led eth1
		}	
		close(fdled);
		
		//led eth2
		fdled = open("/sys/class/net/eth1/carrier", O_RDONLY);	
		if (fdled == -1)
		{
			LedTRMsup &= ~LED_ETH1; //0x00040; //off led eth2
		}
		else
		{
			char buffer;
			read(fdled, &buffer, sizeof(buffer));
			if (buffer == '1') 
				LedTRMsup |= LED_ETH1; //0x00040; //on led eth2
			else
				LedTRMsup &= ~LED_ETH1; //0x00040; //off led eth2
		}	
		close(fdled);
		
		//led wifi
		fdled = open("/sys/class/net/wlan0/carrier", O_RDONLY);	
		if (fdled == -1)
		{
			LedTRMsup &= ~LED_WIFI; //0x00080; //off led wifi
		}
		else
		{
			char buffer;
			read(fdled, &buffer, sizeof(buffer));
			if (buffer == '1') 
				LedTRMsup |= LED_WIFI; //0x00080; //on led wifi
			else
				LedTRMsup &= ~LED_WIFI; //0x00080; //off led wifi
		}	
		close(fdled);
		
		//led EXP CAN0
		fdled = open("/sys/class/net/can0/flags", O_RDONLY);	
		if (fdled == -1)
		{
			//CAN0  non esiste
			LedTRMsup &= ~LED_EXP; //off led exp
			fcan0open = false;
		}
		else
		{	//CAN0 esiste
			char buffer[16]{0};
			read(fdled, &buffer, sizeof(buffer));
			int val = (int)strtol(buffer, NULL, 0);
			if (val == 0x40081) 
			{	//CAN0 aperto qualquno lo usa
				fcan0open = true;
				if (Timercan0errdet > 0) Timercan0errdet--;
				LedTRMsup |= LED_EXP;  //on led exp
			}
			else
			{   //CAN0 chiuso nessuno lo usa
				Timercan0errdet = 2+1; //due secondi
				fcan0open  = false;
				if (fcan0error == false)
				{
					LedTRMsup &= ~LED_EXP; //0x00001; //off led exp
					if(FileExist("/tmp/CAN0close"))  //file generato da script codesys
					{  
						remove("/tmp/CAN0close");
						LedTRMsup |= LED_EXP;   //on led exp
						LedBLKsup |= LED_EXP;   //on led exp lamp.
						LedTRMinf |= LED_E_EXP; //on led error.exp
						dmesgDebug("EXP-CAN0 error , line open", "");
						fcan0error = true;
					}
				}
			}
		}	
		close(fdled);
		
		//led CAN1
		fdled = open("/sys/class/net/can1/flags", O_RDONLY);	
		if (fdled == -1)
		{	//CAN1  non esiste
			LedTRMsup &= ~LED_CAN1; //off led can1
			fcan1open = false;
		}
		else
		{	//CAN1 esiste
			char buffer[16]{0};
			read(fdled, &buffer, sizeof(buffer));
			int val = (int)strtol(buffer, NULL, 0);
			if (val == 0x40081) 
			{  //CAN1 aperto qualquno lo usa
				fcan1open = true;
				if (Timercan1errdet > 0) Timercan1errdet--;
				LedTRMsup |= LED_CAN1; //on led can1
			}
			else
			{	//CAN1 chiuso nessuno lo usa
				Timercan1errdet = 2 + 1; //due secondi
				fcan1open = false;
				if (fcan1error == false) LedTRMsup &= ~LED_CAN1; //0x00002; //off led can1
			}
		}	
		close(fdled);
	
		// gestione errori CAN
		if (GestioneErroriCan==true)
		{
			if (RdFileGestioneErroriCan == true)
			{
				if (FileExist("/data/disablecanerror") == true)
				{
					GestioneErroriCan = false;
					return;
				}
				RdFileGestioneErroriCan = false;
			}
			
			if (TipoBoard == BOARD335)
			{
				if (fcan0open == false)
				{
					LedBLKsup &= ~LED_EXP; //0x00001; //off led can0
					LedTRMinf &= ~LED_E_EXP; //0x00400; //off led e.exp
				}
				else
				{
					LedTRMinf &= ~LED_E_EXP; //0x00400; //off led e.exp
				}	
			}
			if (TipoBoard == BOARD340)
			{
				//led  E.EXP CAN0
				if ((Dip & BIT_DIP1_STOP) == 0) //entra se in RUN
				{
					if ((Timercan0errdet == 0) && (fcan0open == true))  //dopo 2 secondi testa una volta se almeno un frame CAN è passato
					{
						//dmesgDebug("Test err can0 start", "");
						Timercan0errdet = -1;
						fdled = open("/sys/class/net/can0/statistics/tx_packets", O_RDONLY);	
						if (fdled == -1 || fcan0open == false)
						{
							LedBLKsup &= ~LED_EXP; //0x00001; //off led can0
							LedTRMinf &= ~LED_E_EXP; //0x00400; //off led e.exp
							dmesgDebug("Set can0 open err.", "");
						}
						else
						{
							char buffer[16]{0};
							read(fdled, &buffer, sizeof(buffer));
							int val = (int)strtol(buffer, NULL, 0);
							if (val == 0) 
							{
								LedBLKsup |= LED_EXP; //on led can0
								LedTRMinf |= LED_E_EXP; //on led e.exp
								system("ip link set down can0");
								fcan0error = true;
								//char str[2];
								//sprintf(str, "%d", OldPackcan0);
								//dmesgDebug("Set down can0 pk=", str);
								dmesgDebug("EXP-CAN0 error , line open", "");
							}
							else
							{
								LedBLKsup &= ~LED_EXP; //0x00001; //off led can0
								LedTRMinf &= ~LED_E_EXP; //0x00400; //off led e.exp
								dmesgDebug("EXP-CAN0 line OK", "");
							}	
						}	
						close(fdled);
						//dmesgDebug("Test err can0 stop", "");
					}
				}
			}
		
			//led E.CAN1
			if ((Dip & BIT_DIP1_STOP) == 0) //entra se in RUN
			{
				if ((Timercan1errdet == 0) && (fcan1open == true)) //dopo 2 secondi testa una volta se almeno un frame CAN è passato
				{
					//dmesgDebug("Test err can1 start", "");
					Timercan1errdet = -1;
					fdled = open("/sys/class/net/can1/statistics/tx_packets", O_RDONLY);	
					if (fdled == -1 || fcan1open == false)
					{
						LedBLKsup &= ~LED_CAN1; //off led can1
						LedTRMinf &= ~LED_E_CAN1; //off led e.can1
						dmesgDebug("Set can1 open err.", "");
					}
					else
					{
						char buffer[16]{0};
						read(fdled, &buffer, sizeof(buffer));
						int val = (int)strtol(buffer, NULL, 0);
						if (val == 0) 
						{
							LedBLKsup |= LED_CAN1; //on led can1
							LedTRMinf |= LED_E_CAN1; //on led e.can1
							system("ip link set down can1");
							fcan1error = true;
							//char str[2];
							//sprintf(str, "%d", OldPackcan1);
							//dmesgDebug("Set down can1 pk=", str);
							dmesgDebug("CAN1 error , line open", "");
						}
						else
						{
							LedBLKsup &= ~LED_CAN1; //0x00002; //off led can1
							LedTRMinf &= ~LED_E_CAN1; //0x00800; //off led e.can1
							//sprintf(str, "%d", val);
							//dmesgDebug("Set can1 pk=", str);
							dmesgDebug("CAN1 line OK", "");
						}
					}
					//dmesgDebug("Test err can1 stop", "");
					close(fdled);
				}
			}
		}
		TimerGestLedEth = 0;
	}
}
