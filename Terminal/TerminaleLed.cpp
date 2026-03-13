#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h> 
#include <termios.h>

#include "TerminaleLed.h"
#include "PLC.h"
#include "Util.h"

/***************************************************************************************************************************/
/*												GESTIONE COMUNUCAZIONE CON TERMINALE									   */
/***************************************************************************************************************************/

#define NUMERO_BYTE_BUFFER_TX_TRM					24
#define NUMERO_BYTE_BUFFER_RX_TRM					512

#define NUMERO_BYTE_CODE01_TX						24
#define NUMERO_BYTE_CODE01_RX						12

#define NUMERO_BYTE_CODE03_TX						16

#define TX_STRING_TRM								0
#define ATTESA_FINE_TRASMISSIONE					1
#define ATTESA_RISPOSTA_TRM							2
#define ANALIZZA_RISPOSTA_TRM						3
#define STOPCOMUNICAZIONE							40


#define N_DATI_RISPOSTA_TRM							3

#define PAUSA_INVIO_NUOVA_STRINGA_TRM				2
#define TEMPO_MAX_INVIO_STRINGA_TRM					10
#define TEMPO_MAX_ATTESA_STRINGA_TRM				10

#define CODE01_TXLED								0
#define CODE03_RXPARETH0							1
#define CODE03_RXPARETH1							2
#define CODE03_RXPARETH2							3
#define CODE03_RXPARETH3							4
#define CODE03_RXPARETH4							5
#define CODE03_RXPARBRG0							6
#define CODE03_RXPARBRG1							7
#define CODE03_RXPARWLA0AP							8
#define CODE03_RXPARWLA0							9

//	VARIABILI PER LA GESTIONE DELLA SERIALE CON IL TERMINALE

int fd;
unsigned char		TxBufferTRM[NUMERO_BYTE_BUFFER_TX_TRM], RxBufferTRM[NUMERO_BYTE_BUFFER_RX_TRM];
int					TipoStringSendTrm=0;
unsigned char		CodOperRx; //Codice Operazione
unsigned char   	*RxPointerTRM;
unsigned char		DatoRxUsart1;
unsigned long	    TickTRM;
unsigned short	    LedBLKsup, LedBLKinf, LedBLKtrmsup = 0, LedBLKtrminf=0;
unsigned short	    LedTRMsup, LedTRMinf;
unsigned short	    LedTRMsupb, LedTRMinfb;
unsigned char		FaseTxRxTRM;
unsigned char		Dip, Rotary;
unsigned char		OldDip=255, OldRotary;
//bool				TerminaleOffline=true;
bool				TerminaleOnlineAtStart = false;
bool				bPrimaLetturaTRM = true;
bool				bPulsantePremutoAlPowerOn;
unsigned long tick = 0;

int countBLK = 0;

struct ETHX
{
	int Stato;
	int DHCP;
	char IP[16];
	char NetMASK[16];
	char GateWay[16];
	char DNS[16];
	char SDNS[16];
	char ETHname[16];
};
static struct ETHX ETHx[7];

struct WLANX
{
	int Stato;
	int HidenNetwork;
	int Channel;
	char SSID[16];
	char Password[32];
	char ETHname[16];
};
static struct WLANX WLAx[2];

int TempolampallLed;
int TempolampallLedinf;
void TimerStopLampeggioAllled(bool starttimer)
{
	if (blampallled) //flag lampeggio tutti led , Settato da FINDER
	{
		if (starttimer == true)TempolampallLed++;
		if (TempolampallLed >= 600)
		{
			blampallled = false;
			TempolampallLed = 0;
		}
	}
}

void TimerStopLampeggioAllledInf(bool starttimer)
{
	if (blampallledinf) //flag lampeggio tutti led , Settato da FINDER
	{
		if (starttimer == true)TempolampallLedinf++;
		if (TempolampallLedinf >= 600 * 3)
		{
			blampallledinf = false;
			TempolampallLedinf = 0;
		}
	}
}


unsigned long HAL_GetTick(void)
{
	return tick;
}
void HAL_IncTick(void)
{
	tick++;
}

void StopDebug_tty0(void)
{
	system("systemctl stop serial-getty@ttyS0.service");
}	

void StartDebug_tty0(void)
{
	system("systemctl start serial-getty@ttyS0.service");
}	

int set_interface_attribs(int fd, int speed, int parity)
{
	struct termios tty;
	if (tcgetattr(fd, &tty) != 0)
	{
#ifdef DEBUG
		printf("error %d from tcgetattr", errno);
#endif // DEBUG
		return -1;
	}

	cfsetospeed(&tty, speed);
	cfsetispeed(&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
	tty.c_cflag |= (CLOCAL | CREAD); // ignore modem controls,// enable reading
	tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;
	
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~IGNBRK; // disable break processing
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
	tty.c_iflag &= ~(INLCR | IGNCR | ICRNL); // 
	
	tty.c_oflag = 0; // no remapping, no delays
	
	tty.c_lflag = 0; // no signaling chars, no echo,
	// no canonical processing
	tty.c_cc[VMIN]  = 0; // read doesn't block
	tty.c_cc[VTIME] = 5; // 0.5 seconds read timeout
	
	if (tcsetattr(fd, TCSANOW, &tty) != 0)
	{
#ifdef DEBUG
		printf("error %d from tcsetattr", errno);
#endif // DEBUG
		return -1;
	}
	return 0;
}

void set_blocking(int fd, int should_block)
{
	struct termios tty;
	memset(&tty, 0, sizeof tty);
	if (tcgetattr(fd, &tty) != 0)
	{
#ifdef DEBUG
		printf("error %d from tggetattr", errno);		  
#endif // DEBUG
		return;
	}
	tty.c_cc[VMIN]  = should_block ? 1 : 0;
	tty.c_cc[VTIME] = 5; // 0.5 seconds read timeout
#ifdef DEBUG
	if (tcsetattr(fd, TCSANOW, &tty) != 0)
		printf("error %d setting term attributes", errno);	  
#endif // DEBUG
}

//unsigned char bufrxtty0[512];
int ReadSerialeLed(void)
{
	memset(&RxBufferTRM, 0, sizeof(RxBufferTRM));
	int n = read(fd, RxBufferTRM, sizeof RxBufferTRM);
	return n;
}


unsigned char HexAsc(unsigned char  val)
{
	if (val < 10) 
		return 0x30 + val;
	else
		return 0x41 + (val-10);
}

unsigned char NibbleBinHex(unsigned char Nibble)
{
	Nibble = Nibble & 0x0F;
	if (Nibble < 10)
		return (Nibble |  0x30);
	else
		return (Nibble + 'A' - 10);
}

void ByteBinHex(unsigned char *Punt, unsigned char Dato)
{
	unsigned char Nibble;
	Nibble = Dato >> 4;
	Nibble = NibbleBinHex(Nibble);
	*Punt = Nibble;
	Punt++;
	Nibble = Dato & 0x0F;
	Nibble = NibbleBinHex(Nibble);
	*Punt = Nibble;
}

void WordBinHex(unsigned char *Punt, unsigned short Dato)
{
	unsigned char Nibble;
	Nibble = Dato >> 12;
	Nibble = NibbleBinHex(Nibble);
	*Punt = Nibble;
	Punt++;
	Nibble = Dato >> 8;
	Nibble = NibbleBinHex(Nibble);
	*Punt = Nibble;
	Punt++;
	Nibble = Dato >> 4;
	Nibble = NibbleBinHex(Nibble);
	*Punt = Nibble;
	Punt++;
	Nibble = Dato & 0x0F;
	Nibble = NibbleBinHex(Nibble);
	*Punt = Nibble;
}

unsigned short CalcolaCheck(unsigned char *Punt, unsigned char NByte)
{
	unsigned char i;
	unsigned short Check = 0;

	for (i = 0; i < NByte; i++)
	{
		Check += *Punt++;
	}
	return (Check);
}

unsigned char NibbleHexBin(unsigned char Nibble)
{
	if (Nibble < 'A')
		return (Nibble & 0x0F);
	else
		return (Nibble - 'A' + 10);
}

unsigned char ByteHexBin(unsigned char *Punt)
{
	unsigned char Nibble, Dato;
	Nibble = *Punt;
	Dato = NibbleHexBin(Nibble) << 4;
	Nibble = *(Punt + 1);
	return (Dato + NibbleHexBin(Nibble));
}

unsigned short ReadWord(unsigned char *Punt)
{
	unsigned short Dato;
	Dato = ByteHexBin(Punt);
	Dato <<= 8;
	return (Dato + ByteHexBin(Punt + 2));
}

unsigned short ReadCheck(unsigned char *Punt)
{
	return (ReadWord(Punt));
}

void TxFrameCode01_TxLedRxStatus()
{
	//start
	TxBufferTRM[0] = 0x0D;
	//codice 01
	TxBufferTRM[1] = 0x30;
	TxBufferTRM[2] = 0x31;
	
	//spegnimento rapido led in fase di STOP plc
	if (TipoPlc == PLCLOGICLAB)
	{
		bool isstop = (Dip & BIT_DIP1_STOP);
		if (isstop == true && blampallled == false)
		{
			LedTRMsup &= ~LED_EXP; //0x00001; //off led exp
			LedTRMsup &= ~LED_CAN1; //0x00002; //off led can1
			LedTRMinf &= ~LED_E_EXP; //0x00400; //off led e.exp
			LedTRMinf &= ~LED_E_CAN1; //0x00800; //off led e.can1
		
			LedTRMsup &= ~LED_COM1; //0x00004; //off led com1
			LedTRMsup &= ~LED_COM2; //0x00008; //off led com2
			LedTRMsup &= ~LED_VPN; //0x00010; //off led com3
		}
	}
		
	//lampeggio LED
	if (countBLK == 0)
	{
		LedTRMsupb = LedTRMsup;
		LedTRMinfb = LedTRMinf;
	}
	else
	{
		LedTRMsupb &= ~LedBLKsup;
		LedTRMinfb &= ~LedBLKinf;
	}
	if (countBLK++ >= 2) countBLK = 0;
			
	//se powerOFF  = Disabilita offLine del terminale spegni tutti led tranne RUN o STOP  seconda del DIP di stop
	if (PlPowerOff == 1)
	{
		if (Dip & BIT_DIP1_STOP)
		{	
			LedTRMsupb = 0x8000;
			LedBLKtrminf = LED_STOP;
			LedTRMinfb = LED_STOP;
		}
		else
		{	
			LedBLKtrmsup = LED_RUN;
			LedTRMsupb = 0x8000 | LED_RUN;
		}	
		LedTRMinfb = 0x0 ;
	}
	
	//lampeggio di tutti i Led Fila Superiore
	if (blampallledsup == false  && blampallled == false)
	{
		//fila superiore
		WordBinHex(&TxBufferTRM[3], LedTRMsupb);
		//stato led fila superiore lampeggio
		WordBinHex(&TxBufferTRM[11], LedBLKtrmsup);
	}	
	else
	{
		WordBinHex(&TxBufferTRM[3], 0x1FF);
		WordBinHex(&TxBufferTRM[11], 0x1FF);
	}	
	
	//lampeggio di tutti i Led Fila Inferiore
	if (blampallledinf == false && blampallled == false)
	{
		//fila inferiore
		WordBinHex(&TxBufferTRM[7], LedTRMinfb);
		//stato led fila inferiore lampeggio 
		WordBinHex(&TxBufferTRM[15], LedBLKtrminf);
	}	
	else
	{
		WordBinHex(&TxBufferTRM[7], 0x1FF);
		WordBinHex(&TxBufferTRM[15], 0x1FF);
	}	
	
	//calcolo crc
	unsigned int crc = 0; 
	int n;
	for (n = 1; n < (NUMERO_BYTE_CODE01_TX - 5); n++)
	{
		crc = crc + TxBufferTRM[n];
	}
	//set checksum 
	TxBufferTRM[19] = HexAsc((crc & 0xF000) >> 12);
	TxBufferTRM[20] = HexAsc((crc & 0xF00) >> 8);
	TxBufferTRM[21] = HexAsc((crc & 0xF0) >> 4);
	TxBufferTRM[22] = HexAsc(crc & 0xF);
	//end
	TxBufferTRM[23] = 0x0A;	
	write(fd, TxBufferTRM, NUMERO_BYTE_CODE01_TX); 
}

bool RxFrameCode01_Status(int nbyterx)
{	
	//verifica se risposta a codice 01
	if (RxBufferTRM[0] == 0x0D && RxBufferTRM[1] == 0x38 && RxBufferTRM[2] == 0x31  && RxBufferTRM[11] == 0x0A  && nbyterx == NUMERO_BYTE_CODE01_RX)
	{
		if (CalcolaCheck(&RxBufferTRM[1], 6) != ReadCheck(&RxBufferTRM[7])) return false;
		
		Dip = ByteHexBin(&RxBufferTRM[3]);
		if(Dip != OldDip)
		{
			OldDip = Dip;
			char str[3]={0};
			sprintf(str, "%d", Dip);
			touchstr("/tmp/Dip", str);
		}	

		if (bPrimaLetturaTRM)
		{
			bPrimaLetturaTRM = false;
			if (Dip & BIT_PULSANTE)
			{
				bPulsantePremutoAlPowerOn = true;
			}
			if (TipoPlc == PLCCODESYS || TipoPlc == PLCLOGICLAB)
			{
				if (Dip & BIT_DIP1_STOP)
				{
					LedTRMinf = LED_STOP; //0x100; // led stop acceso
					LedBLKinf = LED_STOP; //0x100; // lampeggiante
				}
				else
				{
					LedTRMsup = LED_RUN; //0x10000; // led run acceso
					LedBLKsup = LED_RUN; //0x10000; // lampeggiante
				}
			}
				
		}
		//TerminaleOffline = false;
		TerminaleOnlineAtStart = true;
		return true;
	}
	return false;
}

void TxFrameCode03_RxPar(int StartPar, int  NPar)
{	
	//start
	TxBufferTRM[0] = 0x0D;
	//codice 03
	TxBufferTRM[1] = 0x30;
	TxBufferTRM[2] = 0x33;
	//parametro partenza
	WordBinHex(&TxBufferTRM[3], StartPar);
	//numero parametri
	WordBinHex(&TxBufferTRM[7], NPar);
	//calcolo crc
	unsigned int crc = 0; 
	int n;
	for (n = 1; n < (NUMERO_BYTE_CODE03_TX - 5); n++)
	{
		crc = crc + TxBufferTRM[n];
	}
	//set checksum 
	TxBufferTRM[11] = HexAsc((crc & 0xF000) >> 12);
	TxBufferTRM[12] = HexAsc((crc & 0xF00) >> 8);
	TxBufferTRM[13] = HexAsc((crc & 0xF0) >> 4);
	TxBufferTRM[14] = HexAsc(crc & 0xF);
	//end
	TxBufferTRM[15] = 0x0A;	
	write(fd, TxBufferTRM, NUMERO_BYTE_CODE03_TX); 
}

bool RxFrameCode03_ParEth(int nbyterx, int StartPar, int  NPar, ETHX * eth)
{	
	//verifica se risposta a codice 03
	unsigned char b1, b2, b3, b4;
	int LenRX = (NPar * 2) + 12;
	//char str[64] = { 0 };
	if (RxBufferTRM[0] == 0x0D && RxBufferTRM[1] == 0x38 && RxBufferTRM[2] == 0x33  && RxBufferTRM[LenRX-1] == 0x0A  && nbyterx == LenRX)
	{
		if (CalcolaCheck(&RxBufferTRM[1], LenRX - 6) != ReadCheck(&RxBufferTRM[LenRX-5])) return false;
		Dip = ByteHexBin(&RxBufferTRM[3]);
		eth->Stato = ByteHexBin(&RxBufferTRM[7]);
		eth->DHCP  = ByteHexBin(&RxBufferTRM[9]);
		b1 = ByteHexBin(&RxBufferTRM[11]);
		b2 = ByteHexBin(&RxBufferTRM[13]);
		b3 = ByteHexBin(&RxBufferTRM[15]) ;
		b4 = ByteHexBin(&RxBufferTRM[17]) ;
		sprintf(eth->IP, "%d.%d.%d.%d", b1, b2, b3, b4);
		b1 = ByteHexBin(&RxBufferTRM[19]);
		b2 = ByteHexBin(&RxBufferTRM[21]);
		b3 = ByteHexBin(&RxBufferTRM[23]);
		b4 = ByteHexBin(&RxBufferTRM[25]);
		//sprintf(eth->NetMASK, "%d.%d.%d.%d", b1, b2, b3, b4);
		sprintf(eth->NetMASK, "%d", b4);
		b1 = ByteHexBin(&RxBufferTRM[27]);
		b2 = ByteHexBin(&RxBufferTRM[29]);
		b3 = ByteHexBin(&RxBufferTRM[31]);
		b4 = ByteHexBin(&RxBufferTRM[33]);
		sprintf(eth->GateWay, "%d.%d.%d.%d", b1, b2, b3, b4);
		b1 = ByteHexBin(&RxBufferTRM[35]);
		b2 = ByteHexBin(&RxBufferTRM[37]);
		b3 = ByteHexBin(&RxBufferTRM[39]);
		b4 = ByteHexBin(&RxBufferTRM[41]);
		sprintf(eth->DNS, "%d.%d.%d.%d", b1, b2, b3, b4);
		b1 = ByteHexBin(&RxBufferTRM[43]);
		b2 = ByteHexBin(&RxBufferTRM[45]);
		b3 = ByteHexBin(&RxBufferTRM[47]);
		b4 = ByteHexBin(&RxBufferTRM[49]);
		sprintf(eth->SDNS, "%d.%d.%d.%d", b1, b2, b3, b4);
		//scrivi stato lan su file
		//strcat(str, "/opt//opt/pmb100-terminal/");
		//strcat(str, eth->ETHname);
		//strcat(str, "_NfcState");
		//FILE* fd = fopen(str ,"rw");
		//putw(eth->Stato,fd);
		return true;
	}
	return false;
}

bool RxFrameCode03_ParWlan0AP(int nbyterx, int StartPar, int  NPar, WLANX * eth)
{	
	//verifica se risposta a codice 03
	unsigned char b1;
	int LenRX = (NPar * 2) + 12;

	if (RxBufferTRM[0] == 0x0D && RxBufferTRM[1] == 0x38 && RxBufferTRM[2] == 0x33  && RxBufferTRM[LenRX - 1] == 0x0A  && nbyterx == LenRX)
	{
		if (CalcolaCheck(&RxBufferTRM[1], LenRX - 6) != ReadCheck(&RxBufferTRM[LenRX - 5])) return false;
		Dip = ByteHexBin(&RxBufferTRM[3]);
		eth->Stato = ByteHexBin(&RxBufferTRM[7]); //p_211
		eth->HidenNetwork = ByteHexBin(&RxBufferTRM[9]); //p_212
		eth->Channel = ByteHexBin(&RxBufferTRM[11]); //p_213

		//SSID  p_271
		int a = 0;
		int n ;
		for (n = 0; n < 16; n++)
		{
			b1 = ByteHexBin(&RxBufferTRM[13 + a]);
			if (b1 == 0) break;
			a = a + 2;
			eth->SSID[n] = b1;
		}
		//Password  p_287
		a = 0;
		for (n = 0; n < 32; n++)
		{
			b1 = ByteHexBin(&RxBufferTRM[13 + 32 + a]);
			if (b1 == 0) break;
			a = a + 2;
			eth->Password[n] = b1;
		}
		return true;
	}
	return false;
}

bool RxFrameCode03_ParWlan0(int nbyterx, int StartPar, int  NPar, WLANX * eth)
{	
	//verifica se risposta a codice 03
	unsigned char b1;
	int LenRX = (NPar * 2) + 12;
	
	if (RxBufferTRM[0] == 0x0D && RxBufferTRM[1] == 0x38 && RxBufferTRM[2] == 0x33  && RxBufferTRM[LenRX - 1] == 0x0A  && nbyterx == LenRX)
	{
		if (CalcolaCheck(&RxBufferTRM[1], LenRX - 6) != ReadCheck(&RxBufferTRM[LenRX - 5])) return false;
		Dip = ByteHexBin(&RxBufferTRM[3]);
		//p270
		eth->Stato = ByteHexBin(&RxBufferTRM[7]); 
		//SSID  p_271
		int a = 0;
		for(int n=0;n<16;n++)
		{
			b1 = ByteHexBin(&RxBufferTRM[9+a]);
			if (b1 == 0) break;
			a = a + 2;
			eth->SSID[n] = b1;
		}
		//Password  p_287
		a = 0;
		for (int n = 0; n < 32; n++)
		{
			b1 = ByteHexBin(&RxBufferTRM[9+32 + a]);
			if (b1 == 0) break;
			a = a + 2;
			eth->Password[n] = b1;
		}
		return true;
	}
	return false;
}

bool VerifyFileNMconnection(char* connname)
{
	char str[255] = { 0 };
	strcat(str, "/etc/NetworkManager/system-connections/");
	strcat(str, connname);
	strcat(str, ".nmconnection");
	if (FileExist(str) == true)
	{
		return true;
	}
	return false;
}

void MakeFileNMconnection(char* connname, char* conntype)
{
	char str[255] = { 0 };
	strcat(str, "nmcli connection add con-name ");
	strcat(str, connname);
	strcat(str, " ifname ");
	strcat(str, connname);
	strcat(str, " type ");
	strcat(str, conntype);
	//"nmcli connection add con-name <connection-name> ifname <device-name> type ethernet"	
	system(str);
}

void VerifyOrMakeFileNMcon(char* connname, char* conntype)
{
	char str[255] = { 0 };
	strcat(str, "/etc/NetworkManager/system-connections/");
	strcat(str, connname);
	strcat(str, ".nmconnection");
	if (FileExist(str) == false)
	{
		str[0] = 0;
		strcat(str, "nmcli connection add con-name ");
		strcat(str, connname);
		strcat(str, " ifname ");
		strcat(str, connname);
		strcat(str, " type ");
		strcat(str, conntype);
		//"nmcli connection add con-name <connection-name> ifname <device-name> type ethernet"	
		system(str);
	}
}


void DeleteConnctionEth(char* ethname)
{
	char str[255] = { 0 };
	str[0] = 0;
	strcat(str, "nmcli connection delete ");		
	strcat(str, ethname);
	system(str);
}
	

void MakeFileNMconnectionBridge(char * ethname,  char* bridge)
{
	char str[128] = { 0 };
	str[0] = 0;
	strcat(str, "nmcli connection add type ethernet slave-type bridge con-name ");		
	strcat(str, ethname);
	strcat(str, " ifname "); 
	strcat(str, ethname);
	strcat(str, " master ");
	strcat(str, bridge);
	system(str);
}

void ApplicaParametri(ETHX eth)
{
	char str[255] = { 0 };
	if (eth.DHCP == 0)	//0=dhcp OFF
	{
		strcat(str, "nmcli connection mod ");
		strcat(str, eth.ETHname);
		strcat(str, " ipv4.method manual ipv4.addresses ");
		strcat(str, eth.IP);
		strcat(str, "/");
		strcat(str, eth.NetMASK);
		strcat(str, " ipv4.gateway ");
		strcat(str, eth.GateWay);
		strcat(str, " ipv4.dns \"");
		strcat(str, eth.DNS);
		strcat(str, " ");
		strcat(str, eth.SDNS);
		strcat(str, "\"");
		system(str);
	}
	else
	{
		strcat(str, "nmcli connection mod ");
		strcat(str, eth.ETHname);
		strcat(str, " ipv4.method auto ipv4.addresses \"\" ipv4.gateway \"\" ipv4.dns \"\"");
		system(str);
	}
}

void ApplicaParametriWlan(WLANX eth)
{
	char str[255] = { 0 };
	strcat(str, "nmcli connection mod ");
	strcat(str, eth.ETHname);
	strcat(str, " ipv4.method auto ipv6.method auto");
	system(str);
}

void ApplicaSubitoModifiche(char * ETHname)
{	char str[64] = { 0 };
	return;
	//applica subito le modifiche
	str[0] = 0;
	strcat(str, "nmcli device reapply ");
	strcat(str, ETHname);
	system(str);
}	

void ApplicaParametriSuETH(ETHX eth)
{
	char str[64] = { 0 };
	char strfilenamenfclan[64] = { 0 };
	char strfilenameConnection[64] = { 0 };
	strcat(strfilenamenfclan, "/data/pmb100/nfc_");					
	strcat(strfilenamenfclan, eth.ETHname);
	strcat(strfilenameConnection, "/etc/NetworkManager/system-connections/");
	strcat(strfilenameConnection, eth.ETHname);
	strcat(strfilenameConnection, ".nmconnection");
	
	if (eth.Stato >= 1) touch("/data/pmb100/nfc_wlan0");
	
	//Lan disabled
	if (eth.Stato == 0)
	{
		if (FileExist(strfilenamenfclan) == true ) //rimuovi connessione solo se creata da nfc
		{
			DeleteConnctionEth(eth.ETHname);
			remove(strfilenamenfclan);
			ApplicaSubitoModifiche(eth.ETHname);
		}
		return;
	}
	//Lan enabled
	if (eth.Stato == 1) 
	{	
		if (VerifyFileNMconnection(eth.ETHname) == false)
		{
			MakeFileNMconnection(eth.ETHname, (char*)"ethernet");			//crea file connessione se non esiste
		}
		else
		{
			if (CercaStrSuFile(strfilenameConnection, (char*)"master=bridge", str) == true) 
			{
				DeleteConnctionEth(eth.ETHname);
				MakeFileNMconnection(eth.ETHname, (char*)"ethernet"); //crea file connessione
			}
		}		
		ApplicaParametri(eth);
		ApplicaSubitoModifiche(eth.ETHname);
		return;
	}
	//Lan enabled su bridge 0
	if (eth.Stato == 2) 
	{
		VerifyOrMakeFileNMcon((char*)"bridge0", (char*)"bridge");
		if (VerifyFileNMconnection(eth.ETHname) == true)
		{
			if (CercaStrSuFile(strfilenameConnection, (char*)"master=bridge", str) == false) 
			{
				DeleteConnctionEth(eth.ETHname);
				MakeFileNMconnectionBridge(eth.ETHname, (char*)"bridge0");
			}
		}
		else
		{
			MakeFileNMconnectionBridge(eth.ETHname, (char*)"bridge0");
		}
		ApplicaSubitoModifiche(eth.ETHname);
		return;
	}
	//Lan enabled su bridge 1
	if (eth.Stato == 3) 
	{
		VerifyOrMakeFileNMcon((char*)"bridge1", (char*)"bridge");
		if (VerifyFileNMconnection(eth.ETHname) == true)
		{
			if (CercaStrSuFile(strfilenameConnection, (char*)"master=bridge", str) == false) 
			{
				DeleteConnctionEth(eth.ETHname);
				MakeFileNMconnectionBridge(eth.ETHname, (char*)"bridge1");
			}
		}
		else
		{
			MakeFileNMconnectionBridge(eth.ETHname, (char*)"bridge1");
		}	
		ApplicaSubitoModifiche(eth.ETHname);
		return;
	}
	//bridge enabled
	if (eth.Stato == 4) 
	{	
//		if (VerifyFileNMconnection(eth.ETHname) == false)
//		{
//			MakeFileNMconnection(eth.ETHname, (char*)"ethernet");			//crea file connessione se non esiste
//		}
		ApplicaParametri(eth);
		ApplicaSubitoModifiche(eth.ETHname);
		return;
	}
}

void ApplicaParametriSuWLANAP(WLANX eth)
{
	char str[128] = { 0 };
	char strfilenamenfclan[64] = { 0 };
	char strfilenameConnection[64] = { 0 };
	strcat(strfilenamenfclan, "/data/pmb100/nfc_wlan0ap");					
	strcat(strfilenameConnection, "/etc/NetworkManager/system-connections/wlan0.nmconnection");
	
	if (eth.Stato >= 1) touch("/data/pmb100/nfc_wlan0ap");
	
	//Lan disabled
	if (eth.Stato == 0)
	{
		if (FileExist(strfilenamenfclan) == true) //rimuovi connessione solo se creata da nfc
		{
			DeleteConnctionEth((char*)"Hotspot");
			remove(strfilenamenfclan);
			ApplicaSubitoModifiche((char*)"wlan0");
		}
		return;
	}
	//Lan enabled
	if (eth.Stato == 1) 
	{	
		if (VerifyFileNMconnection((char*)"Hotspot") == false)
		{
			//crea file connessione se non esiste
			strcat(str, "nmcli d wifi hotspot ifname wlan0 ssid "); //testspot password 12345678
			strcat(str, eth.SSID);
			strcat(str, " password ");
			strcat(str, eth.Password);
			system(str);
		}
		else
		{
			DeleteConnctionEth((char*)"Hotspot");
			strcat(str, "nmcli d wifi hotspot ifname wlan0 ssid "); //testspot password 12345678
			strcat(str, eth.SSID);
			strcat(str, " password ");
			strcat(str, eth.Password);
			system(str);
		}	
			
		system("nmcli connection mod Hotspot autoconnect true");
		ApplicaSubitoModifiche((char*)"wlan0");
		return;
	}
}

void ApplicaParametriSuWLAN(WLANX eth)
{
	char str[128] = { 0 };
	char strfilenamenfclan[64] = { 0 };
	char strfilenameConnection[64] = { 0 };
	strcat(strfilenamenfclan, "/data/pmb100/nfc_");					
	strcat(strfilenamenfclan, eth.ETHname);
	strcat(strfilenameConnection, "/etc/NetworkManager/system-connections/");
	strcat(strfilenameConnection, eth.ETHname);
	strcat(strfilenameConnection, ".nmconnection");
	
	//Lan disabled
	if (eth.Stato == 0)
	{
		//rimuovi connessione solo se creata da nfc
		if (FileExist(strfilenamenfclan) == true) 
		{
			DeleteConnctionEth((char*)"wlan0");
			remove(strfilenamenfclan);
			ApplicaSubitoModifiche((char*)"wlan0");
		}
		return;
	}
	//Lan enabled
	if (eth.Stato == 1) 
	{	
		if (VerifyFileNMconnection((char*)"wlan0") == false)
		{
			strcat(str, "nmcli connection add con-name wlan0 ifname wlan0 type wifi ssid "); //testspot password 12345678
			strcat(str, eth.SSID);
			strcat(str, " wifi-sec.key-mgmt wpa-psk wifi-sec.psk \"");
			strcat(str, eth.Password);
			strcat(str, "\"");
			system(str);
		}
		else
		{
			DeleteConnctionEth((char*)"wlan0");
			strcat(str, "nmcli connection add con-name wlan0 ifname wlan0 type wifi ssid "); //testspot password 12345678
			strcat(str, eth.SSID);
			strcat(str, " wifi-sec.key-mgmt wpa-psk wifi-sec.psk \"");
			strcat(str, eth.Password);
			strcat(str, "\"");
			system(str);
		}
			
		ApplicaParametriWlan(eth);
		ApplicaSubitoModifiche((char*)"wlan0");
		return;
	}
}

void ApplicaParametriNFC()
{
	ApplicaParametriSuETH(ETHx[0]); 
	ApplicaParametriSuETH(ETHx[1]); if (ETHx[1].Stato >= 1) touch("/data/pmb100/nfc_eth1");
	ApplicaParametriSuETH(ETHx[2]); if (ETHx[2].Stato >= 1) touch("/data/pmb100/nfc_eth2");
	ApplicaParametriSuETH(ETHx[3]); if (ETHx[3].Stato >= 1) touch("/data/pmb100/nfc_eth3");
	ApplicaParametriSuETH(ETHx[4]); if (ETHx[4].Stato >= 1) touch("/data/pmb100/nfc_eth4");
	ApplicaParametriSuWLANAP(WLAx[0]);
	ApplicaParametriSuWLAN(WLAx[1]);
	
	//bridge0
	if (ETHx[0].Stato == 2 || ETHx[1].Stato == 2 || ETHx[2].Stato == 2 || ETHx[3].Stato == 2 || ETHx[4].Stato == 2 || WLAx[1].Stato == 2)
	{
		ETHx[5].Stato = 4;
		ApplicaParametriSuETH(ETHx[5]);
	}
	else
	{
		if (VerifyFileNMconnection(ETHx[5].ETHname) == true)
		{
			DeleteConnctionEth(ETHx[5].ETHname);
			ApplicaSubitoModifiche(ETHx[5].ETHname);
		}
	}
	
	//bridge1
	if (ETHx[0].Stato == 3 || ETHx[1].Stato == 3 || ETHx[2].Stato == 3 || ETHx[3].Stato == 3 || ETHx[4].Stato == 3  || WLAx[1].Stato == 3)
	{
		ETHx[6].Stato = 4;
		ApplicaParametriSuETH(ETHx[6]);
	}
	else
	{
		if (VerifyFileNMconnection(ETHx[6].ETHname) == true)
		{
			DeleteConnctionEth(ETHx[6].ETHname);
			ApplicaSubitoModifiche(ETHx[6].ETHname);
		}
	}
	
	//applica parametri globalmente riavviando  NetworkManager
	if (ETHx[0].Stato != 0 || ETHx[1].Stato != 0 || ETHx[2].Stato != 0 || ETHx[3].Stato != 0 || ETHx[4].Stato != 0 || WLAx[0].Stato != 0 || WLAx[1].Stato != 0)
	{
		system("systemctl restart NetworkManager.service");
	}
	
}
void ComunicazioneConTRM(void)
{
	switch (FaseTxRxTRM)
	{
	case STOPCOMUNICAZIONE	:	//	stop comunicazione per power off
		break;
		
	case TX_STRING_TRM				:	//	Trasmetti stringa al terminale
		if ((HAL_GetTick() - TickTRM) > PAUSA_INVIO_NUOVA_STRINGA_TRM)
		{
			if (TipoStringSendTrm == CODE01_TXLED) TxFrameCode01_TxLedRxStatus();
			if (TipoStringSendTrm == CODE03_RXPARETH0) TxFrameCode03_RxPar(1,22);
			if (TipoStringSendTrm == CODE03_RXPARETH1) TxFrameCode03_RxPar(31, 22);
			if (TipoStringSendTrm == CODE03_RXPARETH2) TxFrameCode03_RxPar(61, 22);
			if (TipoStringSendTrm == CODE03_RXPARETH3) TxFrameCode03_RxPar(91, 22);
			if (TipoStringSendTrm == CODE03_RXPARETH4) TxFrameCode03_RxPar(121, 22);
			if (TipoStringSendTrm == CODE03_RXPARBRG0) TxFrameCode03_RxPar(152, 22);
			if (TipoStringSendTrm == CODE03_RXPARBRG1) TxFrameCode03_RxPar(182, 22);
			if (TipoStringSendTrm == CODE03_RXPARWLA0AP) TxFrameCode03_RxPar(211, 3+16+32);
			if (TipoStringSendTrm == CODE03_RXPARWLA0) TxFrameCode03_RxPar(270, 1+16+32);

			FaseTxRxTRM = ATTESA_FINE_TRASMISSIONE;
			//stop comunicazione per power off
			//if (PlPowerOff == 1) FaseTxRxTRM = STOPCOMUNICAZIONE;
			TickTRM = HAL_GetTick();
		}
		break;

	case ATTESA_FINE_TRASMISSIONE	:	//	Attendi fine trasmissione al terminale
		if ((HAL_GetTick() - TickTRM) > TEMPO_MAX_INVIO_STRINGA_TRM)
		{
			//FaseTxRxTRM = TX_STRING_TRM;
			//TerminaleOffline = TRUE;
			FaseTxRxTRM = ANALIZZA_RISPOSTA_TRM;
			TickTRM = HAL_GetTick();
		}
		break;


	case ANALIZZA_RISPOSTA_TRM		:	//	Analizza dati ricevuti dal terminale
		int nbyterx = ReadSerialeLed();
		
		switch (TipoStringSendTrm)  
		{	
			//Lettura Sattus DIP/NFC
			case CODE01_TXLED		:RxFrameCode01_Status(nbyterx); break;
			//Lettura Parametri NFC
			case CODE03_RXPARETH0	:strcpy(ETHx[0].ETHname, "eth0");    if (RxFrameCode03_ParEth(nbyterx, 1,  22, &ETHx[0]) == true) { TipoStringSendTrm++; } break;
			case CODE03_RXPARETH1	:strcpy(ETHx[1].ETHname, "eth1");    if (RxFrameCode03_ParEth(nbyterx, 31, 22, &ETHx[1]) == true) { TipoStringSendTrm++; } break;
			case CODE03_RXPARETH2	:strcpy(ETHx[2].ETHname, "eth2");    if (RxFrameCode03_ParEth(nbyterx, 61, 22, &ETHx[2]) == true) { TipoStringSendTrm++; } break;
			case CODE03_RXPARETH3	:strcpy(ETHx[3].ETHname, "eth3");    if (RxFrameCode03_ParEth(nbyterx, 91, 22, &ETHx[3]) == true) { TipoStringSendTrm++; } break;
			case CODE03_RXPARETH4	:strcpy(ETHx[4].ETHname, "eth4");    if (RxFrameCode03_ParEth(nbyterx, 121, 22, &ETHx[4]) == true) { TipoStringSendTrm++; } break;
			case CODE03_RXPARBRG0	:strcpy(ETHx[5].ETHname, "bridge0"); if (RxFrameCode03_ParEth(nbyterx, 152, 22, &ETHx[5]) == true) { TipoStringSendTrm++; } break;
			case CODE03_RXPARBRG1	:strcpy(ETHx[6].ETHname, "bridge1"); if (RxFrameCode03_ParEth(nbyterx, 182, 22, &ETHx[6]) == true) { TipoStringSendTrm++;  } break;
			case CODE03_RXPARWLA0AP	:strcpy(WLAx[0].ETHname, "wlan0"); if (RxFrameCode03_ParWlan0AP(nbyterx, 211, 3+16+32, &WLAx[0]) == true) { TipoStringSendTrm++; } break;
			case CODE03_RXPARWLA0	:strcpy(WLAx[1].ETHname, "wlan0"); if (RxFrameCode03_ParWlan0(nbyterx, 270,1+16+32, &WLAx[1]) == true)   {TipoStringSendTrm = CODE01_TXLED;ReadParNFCok = true;}break;
		}
			
		//abilita lettura parametri NFC
		if (ReadParNFC==true)
		{	
			ReadParNFC = false;
			TipoStringSendTrm = CODE03_RXPARETH0;
		}	
		
		TickTRM = HAL_GetTick();
		FaseTxRxTRM = TX_STRING_TRM;
		break;
	}
	HAL_IncTick();
}

bool Fstopthled=true;
void *th_led(void * arg) //task 10ms
{
	while (Fstopthled)
	{
		ComunicazioneConTRM();
		msleep(10);
	}
	pthread_exit(0);
}

void InizializzaTerminaleLedUart(bool stato)
{	
	pthread_attr_t tattr;
	pthread_t thled;
	int ret;
	int newprio = 18;
	sched_param param;
	
	if (stato == false)
	{	
		Fstopthled = false;
		msleep(50);
		StartDebug_tty0();
		return;
	}
	StopDebug_tty0();
	
	char *portname  = (char*) "/dev/ttyS0";
	fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0)
	{
#ifdef DEBUG
		printf("error %d opening %s: %s\n", errno, portname, strerror(errno)); 
#endif // DEBUG
		return;
	}
	//set_interface_attribs(fd, B38400,PARENB); // set speed to 38400 bps, 8E1 (even parity)
	set_interface_attribs(fd, B115200, 0); // set speed to 57600 bps, 8N1 (no parity)
	set_blocking(fd, 0); // set no blocking

	FaseTxRxTRM = TX_STRING_TRM;
	TickTRM = HAL_GetTick();
	
	LedTRMsup = 0x00; 
	LedBLKsup = 0x00;	
	LedTRMinf = 0x00; 
	LedBLKinf = 0x00;	
	
	/* initialized with default attributes */
	ret = pthread_attr_init(&tattr);
	/* safe to get existing scheduling param */
	ret = pthread_attr_getschedparam(&tattr, &param);
	/* set the priority; others are unchanged */
	param.sched_priority = newprio;
	/* setting the new scheduling param */
	ret = pthread_attr_setschedparam(&tattr, &param);
	/* with new priority specified */
	int status = pthread_create(&thled, &tattr, th_led, NULL);
}

void InizializzaTerminaleLedSpi(void)
{
	
}
