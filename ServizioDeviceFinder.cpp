#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/reboot.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/statvfs.h>
#include <sys/inotify.h>

#include <TerminaleLed.h>
#include <ServizioDeviceFinder.h>
#include <Util.h>

#define PORT 12346	//The port on which to listen for incoming data
#define BUFLEN 512	//Max length of buffer

struct sockaddr_in si_me, si_other;
	
int s_finder, i, recv_len;
socklen_t slen = sizeof(si_other);
char buf[BUFLEN];
char bufhalf[BUFLEN / 2];

char par[10][32];

static char DEV[16] {0};
static char MAC[13] {0};

bool blampallled;
bool blampallledinf;
bool blampallledsup;
int RitardoStartFinder = 100; //200; //10 secondi

extern void StopPLC(void);
extern void StartPLC(void);
void GestioneUdpFinder(void);
extern int bytesecondDISK_wr;
extern char SerialNumber[];
extern char StrTipoPlc[];
extern bool LicPresent;

void *th_finder(void * arg) //task ricezione
{
	while (true)
	{
		GestioneUdpFinder();
	}
	pthread_exit(0);
}

void mac_eth(char MAC_str[13],char ethname[5])
{
#define HWADDR_len 6
	int s, i;
	struct ifreq ifr;
	s = socket(AF_INET, SOCK_DGRAM, 0);
	strcpy(ifr.ifr_name, ethname);
	ioctl(s, SIOCGIFHWADDR, &ifr);
	for (i = 0; i < HWADDR_len; i++)
		sprintf(&MAC_str[i * 2], "%02X", ((unsigned char*)ifr.ifr_hwaddr.sa_data)[i]);
	MAC_str[12] = '\0';
	close(s);
}

//void GetMacFromIP(char ipinuse[16])
//{
//	FILE *fp = fopen("/proc/net/arp", "r");
//	char ip[99], hw[99], flags[99], mac[99], mask[99], dummy[99];
//	fgets(dummy, 99, fp); //header line
//	while (fscanf(fp, "%s %s %s %s %s %s\n", ip, hw, flags, mac, mask, DEV) != EOF)
//	{}	
//	//	if (!strcmp(ipinuse, ip))
//	//		printf("%s\n", DEV);
//	fclose(fp);
//	mac_eth(MAC, DEV);
//}	

char arpfile[2048];
char oarpfile[2048];
void GetARPfile(char * buffer)
{
	int num_chars = 0;
	FILE *fp = fopen("/proc/net/arp", "r");
	for (int ch = fgetc(fp); ch != EOF; ch = fgetc(fp)) {
		buffer[num_chars++] = ch;
	}
	// null-terminate the string
	buffer[num_chars] = '\0';
	fclose(fp);
}		

void GetDEVname(char ipsor[16],char * devname)
{
	FILE *fp = fopen("/proc/net/arp", "r");
	char ip[16] {0}, hw[99], flags[99], mac[99], mask[99], dummy[99];
	fgets(dummy, 99, fp); //header line
	while (fscanf(fp, "%s %s %s %s %s %s\n", ip, hw, flags, mac, mask, devname) != EOF)
	{
		if (strcmp(ipsor, ip) == 0)
		{
			fclose(fp);
			return;
		}	
	}	
	fclose(fp);
}	


void GetMACDEVname(char ipsor[16])
{
	FILE *fp = fopen("/proc/net/arp", "r");
	char ip[16] {0}, hw[99], flags[99], mac[99], mask[99], dummy[99];
	fgets(dummy, 99, fp); //header line
	while (fscanf(fp, "%s %s %s %s %s %s\n", ip, hw, flags, mac, mask, DEV) != EOF)
	{
		if(strcmp(ipsor,ip)==0 )
		{
			mac_eth(MAC, DEV);
			break;
		}	
	}	
	fclose(fp);
}	


bool InitFinder = false;
int TimerRitRunFinder = 0;

void InizializzaFinder(void)
{	
	pthread_t thfinder;
	mac_eth(MAC,(char*)"eth0");
	
	//create a UDP socket
	if ((s_finder = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		return;
	}
	// zero out the structure
	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);  //INADDR_BROADCAST
	//bind socket to port
	if (bind(s_finder, (struct sockaddr*)&si_me, sizeof(si_me)) == -1)
	{
		return;
	}
	int status = pthread_create(&thfinder, NULL, th_finder, NULL);
	if (status < 0)
	{
		exit(1);
	}
	InitFinder = true;
}


void GestioneFinder()
{
#ifdef FINDER
	if (InitFinder == false)
	{
		TimerRitRunFinder++;
		if (TimerRitRunFinder >= RitardoStartFinder)  //20 secondi
		{
			//StartExternalFinder();
			InizializzaFinder();
			InitFinder = true;
		}
	}
#endif
}

void Utf16toChar(char* input, char* output, int length_in) {
	int j = 0;
	int i = 0;
	for (i; i < length_in; i += 2)  //Step over 2 bytes
	{
		output[j] = input[i];	
		if (output[j] == 0)
			break;
		j++;
	}
}

short charToUtf16(char* input, char* output, int length_out) {
	int j = 0;
	short i;
	for (i = 0; i < length_out; i += 2)  //Step over 2 bytes
	{
		//Lets use little-endian - smallest bytes first
		output[i] = input[j];
		output[i + 1] = 0; //We will never have any data for this field
		if (input[j] == 0)
			break;
		j++;
	}
	return i + 2;
}

long GetAvailableSpace(const char* path)
{
	struct statvfs stat;
	if (statvfs(path, &stat) != 0) {
		// error happens, just quits here
		return -1;
	}
	// the available size is f_bsize * f_bavail
	return stat.f_bsize * stat.f_bavail;
}


char hostname[HOST_NAME_MAX + 1];
struct sysinfo info;

void SendStatusPL(void)
{
	//campo OS_Ver su finder
	char VEROS[100] =  {0};
	strcat(VEROS,StrTipoPlc);
	strcat(VEROS,SerialNumber);
	strcat(VEROS,"_");
	strcat(VEROS, DEV);
	//campo Memory free su finder
	sysinfo(&info);
	float freeramMB = (float)info.freeram / (1024*1024);
	int freedskMB = GetAvailableSpace((char*)"/") / (1024*1024); //in MB
	char MemoryFree[64] = {0};
	if (LifeTimeMMC_1to10 != 0)
		snprintf(MemoryFree, sizeof(MemoryFree), "D=%03dMB:%.2fkb/s LT=%d%% R=%3.3fMB", freedskMB, (double)bytesecondDISK_wr / 1024, LifeTimeMMC_1to10 * 10, freeramMB);
	else
		snprintf(MemoryFree, sizeof(MemoryFree), "D=%03dMB:%.2fkb/s R=%3.3fMB", freedskMB, (double)bytesecondDISK_wr / 1024, freeramMB);
	//campo Time
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	//campo Device_Name
	gethostname(hostname, HOST_NAME_MAX + 1);
	char name[512] = { 0 };
	//make string TX
	snprintf(name, sizeof(name), "%s  ,%s , %s , %s , %02d/%02d/%d %02d:%02d:%02d ,OK, %s", "FOUND_BRO", hostname, VEROS, MAC, tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec, MemoryFree);
	//convert in unicode 16bit
	int size_utf16 = strlen(name) * 2;
	char name_utf_16[512*2]; 
	charToUtf16(name, name_utf_16, size_utf16);
	//send 
	if (sendto(s_finder, name_utf_16, size_utf16, 0, (struct sockaddr*) &si_other, slen) == -1)
	{
		return;
	}
}

void DebugRxMsg(int nmsgprint)
{
	int n;
	printf("RX from %s:%d ", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
	printf("Data: ");
	for(n=0;n<nmsgprint;n++)
		printf("%s,",&par[n][0]);
	printf("\n");
}	

//function to set date and time
void setDateTime(int date, int month, int year, int hour, int min, int sec)
{
	//buffer to format command
	unsigned char buff[32] = { 0 };
	//formatting command with the given parameters
	sprintf((char*)buff, (const char *)"date -s \"%02d/%02d/%04d %02d:%02d:%02d\"", month, date, year, hour, min, sec);
	//execute formatted command using system()
	system((const char *)buff);
	system("hwclock -w");
}


int splitpar(char str[])
{
	unsigned count = 0;
	/* First call to strtok should be done with string and delimiter as first and second parameter*/
	char *sep = (char*)",";
	char *token = strtok(str, sep);
	//count++;

	/* Consecutive calls to the strtok should be with first parameter as NULL and second parameter as delimiter
	 * * return value of the strtok will be the split string based on delimiter*/
	while (token != NULL)
	{
		strcpy(&par[count][0], token);
		token = strtok(NULL, sep);
		count++;
	}
	return count;
}

char  ipsorg[16];
char  o_ipsorg[16];
int npackrx=0;
void GestioneUdpFinder (void)
{
	//printf("Waiting... ");
	memset(buf, 0, sizeof(buf));
	memset(bufhalf, 0, sizeof(bufhalf));
	//try to receive some data, this is a blocking call
	if ((recv_len = recvfrom(s_finder, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1) return;
	Utf16toChar(buf, bufhalf, recv_len);
	int npar = splitpar(bufhalf);	
	strcpy(ipsorg,inet_ntoa(si_other.sin_addr));
	//if(strcmp(ipsorg,o_ipsorg) != 0)
	//{	
	//	strcpy(o_ipsorg, ipsorg);
		GetDEVname(ipsorg,DEV);
		mac_eth(MAC, DEV);
	//}
	if ((strcmp(&par[0][0],"WHOAREYOU:") == 0) && (npar == 2))
	{	
		//DebugRxMsg(npar);
		SendStatusPL();
		return;
	}
	
	if (strcmp(&par[0][0],"XMASTREE*:") == 0 && (npar==3))
	{	
		//estrai MAC di destinazione
		if (strcmp(&par[1][0],MAC)==0)		      
		{									//questo è per me
			//DebugRxMsg(npar);
			if (strcmp(&par[2][0], "1") == 0)
				blampallled = true;			//lampeggia tutti i led
			else
				blampallled = false;		//off lampeggio di tutti i led
		}
		return;
	}
	
	if (strcmp(&par[0][0], "FIND____?:") == 0)
	{	
		DebugRxMsg(npar);
	}
	
	if (strcmp(&par[0][0], "TIME*****:") == 0)
	{	
		DebugRxMsg(npar);
//		struct tm time = { 0 };
//		
//		time.tm_year = atoi(&par[6][0]) - 1900;
//		time.tm_mon  = atoi(&par[5][0]) - 1;
//		time.tm_mday = atoi(&par[4][0]);
//		time.tm_hour = atoi(&par[1][0]);
//		time.tm_min  = atoi(&par[2][0]);
//		time.tm_sec  = atoi(&par[3][0]);
//		time_t t = mktime(&time);
//		if(t != (time_t) - 1)
//		{
//			settimeofday(&t);
//		}
		setDateTime(atoi(&par[4][0]), atoi(&par[5][0]), atoi(&par[6][0]), atoi(&par[1][0]), atoi(&par[2][0]), atoi(&par[3][0]));
		return;
	}
	
	if (strcmp(&par[0][0], "RUN******:") == 0)
	{	
		DebugRxMsg(npar);
		if (strcmp(&par[1][0], "codesyscontrol.service") == 0)
		{
			StartPLC();
			return;
		}
		unsigned char buff[64] = { 0 };
		sprintf((char*)buff, (const char *)"systemctl start %s", &par[1][0]);
		system((const char *)buff);
		return;
	}
	
	if (strcmp(&par[0][0], "KILL*****:") == 0)
	{	
		DebugRxMsg(npar);
		if (strcmp(&par[1][0], "codesyscontrol.service") == 0)
		{
			StopPLC();
			return;
		}
		unsigned char buff[64] = { 0 };
		sprintf((char*)buff, (const char *)"systemctl stop %s", &par[1][0]);
		system((const char *)buff);
		return;
	}
	
	if (strcmp(&par[0][0], "RESET****:") == 0)
	{	
		DebugRxMsg(npar);
	}
	
	if (strcmp(&par[0][0], "RESETM***:") == 0 && (npar == 2))
	{	
		//estrai MAC di destinazione
		if (strcmp(&par[1][0], MAC) == 0)		      
		{
			//questo è per me
			DebugRxMsg(npar);
			system("reboot");
		}
		return;
	}
	
	if (strcmp(&par[0][0], "SETIP****:") == 0)
	{	
		DebugRxMsg(npar);
	}
	
	if ((strcmp(&par[0][0], "GETIP****:") == 0) && (npar == 2))
	{	
		//estrai MAC di destinazione
		if (strcmp(&par[1][0],MAC)==0)		      
		{									//questo è per me
			DebugRxMsg(npar);
		}
	}
	
	if (strcmp(&par[0][0], "SD>EMMC**:") == 0)
	{	
		DebugRxMsg(npar);
	}
	
	if (strcmp(&par[0][0], "SETWLAN**:") == 0)
	{	
		DebugRxMsg(npar);
	}
	
	if (strcmp(&par[0][0], "NETSTAT**:") == 0)
	{	
		DebugRxMsg(npar);
	}
	
}


//void SendStatusTD(int u)
//{
//	TIME_ZONE_INFORMATION ti;
//	GetTimeZoneInformation(&ti);
//	int dl = ti.DaylightBias;
//	MEMORYSTATUS memstat;
//	memstat.dwLength = sizeof(memstat);
//	//riempi struttura con stato memoria RAM
//	GlobalMemoryStatus(&memstat);
//	//riempi struttura con Local TIME
//	SYSTEMTIME st;
//	GetLocalTime(&st);
//	char name[255] = { 0 };
//	sprintf_s(name, sizeof(name), "%s  ,%s , %s , %s , %d/%d/%d %d:%0d:%0d ,OK, :%d", "FOUND_BRO", NAME, VEROS, MAC, st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond, memstat.dwAvailPhys);
//	int size_utf16 = strlen(name) * 2;
//	char name_utf_16[255]; 
//	charToUtf16(name, name_utf_16, size_utf16);
//	char *ip = inet_ntoa(SenderAddr.sin_addr);
//	if (u == 1)
//		TXinfo.sin_addr.s_addr = inet_addr(ip); ///< IP Address of sender
//	else
//		TXinfo.sin_addr.s_addr = inet_addr("255.255.255.255");
//	retVal = sendto(TXSocket, name_utf_16, size_utf16, 0, (SOCKADDR *) & TXinfo, sizeof(TXinfo));
//	if (retVal == SOCKET_ERROR)
//	{
//		RETAILMSG(1, (TEXT("\nCould not send message to Server with error code : %d"), WSAGetLastError()));
//	}
//}



//void GestioneFinder(void)
//{
	
//	//try to receive some data, this is a blocking call
//	if ((recv_len = recvfrom(s_finder, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
//	{
//		return;
//	}
//	FillMemory(RXBufferCHAR,sizeof(RXBufferCHAR),0);
//	Utf16toChar(RXBufferUTF16,RXBufferCHAR,sizeof(RXBufferUTF16));
//	char * way = "WHOAREYOU:"; //cerca tutti in broadcast , tutti rispondono in broadcast
//	char * fin = "FIND____?:"; //risponde in unicast tipo 1
//	char * tim = "TIME*****:"; //risponde in unicast tipo 0
//	char * run = "RUN******:";
//	char * kil = "KILL*****:";
//	char * res = "RESET****:";
//	char * lc0 = "LAN>CAN0*:";
//	char * lc1 = "LAN>CAN1*:";
//	char * ls2 = "LAN>COM2*:";
//	char * ls1 = "LAN>COM1*:";
//	char * mac = "SETMAC***:";
//	char * sip = "SETIP****:";
//	char * si2 = "SETIP2***:";
//	char * gip = "GETIP****:"; //risponde in broadcast
//	char * uip = "GETIPU***:"; //risponde in unicast
//	char * gi2 = "GETIP2***:"; //risponde in broadcast
//	char * ui2 = "GETIPU2**:"; //risponde in unicast
//	char * log = "LOGPLC***:";
//
//	char seps[] = ",";
//	char par[10][256];
//	char *token;
//	FillMemory(par,sizeof(par),0);
//	token = strtok( RXBufferCHAR, seps );
//	if (token!=NULL)
//	{
//		int j=0;
//		while( token != NULL )
//		{
//			/* While there are tokens in "string" */
//			//printf( " %s\n", token );
//			/* Get next token: */
//			strcpy(&par[j][0],token);
//			token = strtok( NULL, seps );
//			j++;
//		}
//		//richiesta status rispondi in broadcast
//		if (strcmp(&par[0][0],way)==0)
//			SendStatusTD(0);
//		//rispondi status rispondi in unicast
//		if (strcmp(&par[0][0],fin)==0)
//			SendStatusTD(1);
//		//se time
//		if (strcmp(&par[0][0],tim)==0)
//		{
//			SYSTEMTIME st;
//			GetLocalTime(&st);
//			st.wHour=strtoul(&par[1][0],NULL,10);
//			st.wMinute=strtoul(&par[2][0],NULL,10);
//			st.wSecond=strtoul(&par[3][0],NULL,10);
//			st.wDay=strtoul(&par[4][0],NULL,10);
//			st.wMonth=strtoul(&par[5][0],NULL,10);
//			st.wYear=strtoul(&par[6][0],NULL,10);
//			SetTimeZoneInformationByID(1340);		//roma
//			RegFlushKey(HKEY_LOCAL_MACHINE);
//			SetLocalTime(&st);
//			SendStatusTD(1);
//		}
//		//run proces
//		if(strcmp(&par[0][0],run)==0)
//		{
//			wchar_t wtext[128];
//			mbstowcs(wtext,&par[1][0] , strlen(&par[1][0])+1);//Plus null
//			LPWSTR ptr = wtext;
//			if (CreateProcess(ptr,NULL,NULL,NULL,NULL,0,NULL,NULL,NULL,NULL))
//				RETAILMSG(1, (TEXT("RUN: %s \r\n"),wtext));
//				
//		}
//		//kill proces
//		if(strcmp(&par[0][0],kil)==0)
//		{
//			wchar_t wtext[128];
//			mbstowcs(wtext,&par[1][0] , strlen(&par[1][0])+1);//Plus null
//			LPWSTR ptr = wtext;
//			CharUpper(ptr);
//			if (KillProcess(ptr))
//				RETAILMSG(1, (TEXT("KILL: %s \r\n"),wtext));
//				
//		}
//		//reset 
//		if(strcmp(&par[0][0],res)==0)
//		{
//			RETAILMSG(1, (TEXT("RESET:\r\n")));
//			SetSystemPowerState(NULL, POWER_STATE_RESET,POWER_FORCE);
//			Sleep(2000);
//		}
//		//setmac 
//		if(strcmp(&par[0][0],mac)==0)
//		{
//			RETAILMSG(1, (TEXT("SETMAC:\r\n")));
//			//SetSystemPowerState(NULL, POWER_STATE_RESET,POWER_FORCE);
//		}
//		//setip 
//		if(strcmp(&par[0][0],sip)==0)
//		{   //se ip presi da dip non settare da file
//			if ((dipdhcp == false) && (dipip==false))
//			{
//				wchar_t mac[128];
//				mbstowcs(mac,&par[1][0] , strlen(&par[1][0])+1);//Plus null
//				//verifica che il comando sia per il mio MAC
//				if (wcscmp(WMAC , mac) == 0)
//				{
//					RETAILMSG(1, (TEXT("SETIP:\r\n")));
//					SetIPFile(&par[2][0]); //scrivi il file IP.TXT per cambio IP
//					SetIPLan();
//					SetSystemPowerState(NULL, POWER_STATE_RESET,POWER_FORCE);
//					Sleep(2000);
//				}
//			}
//		}
//		//getip 
//		if(strcmp(&par[0][0],gip)==0)
//		{
//			wchar_t mac[128];
//			mbstowcs(mac,&par[1][0] , strlen(&par[1][0])+1);//Plus null
//			//verifica che il comando sia per il mio MAC
//			if (wcscmp(WMAC , mac) == 0)
//			{
//				RETAILMSG(1, (TEXT("GETIP:\r\n")));
//				SendGetIP(0);
//			}
//		}
//		//getip unicast
//		if(strcmp(&par[0][0],uip)==0)
//		{
//			wchar_t mac[128];
//			mbstowcs(mac,&par[1][0] , strlen(&par[1][0])+1);//Plus null
//			//verifica che il comando sia per il mio MAC
//			if (wcscmp(WMAC , mac) == 0)
//			{
//				RETAILMSG(1, (TEXT("GETIPU:\r\n")));
//				SendGetIP(1);
//			}
//		}
//
//		//setip2
//		if(strcmp(&par[0][0],si2)==0)
//		{   
//			wchar_t mac[128];
//			mbstowcs(mac,&par[1][0] , strlen(&par[1][0])+1);//Plus null
//			//verifica che il comando sia per il mio MAC
//			if (wcscmp(WMAC , mac) == 0)
//			{
//				RETAILMSG(1, (TEXT("SETIP2:\r\n")));
//				SetIP2File(&par[2][0]); //scrivi il file IP.TXT per cambio IP
//				SetIP2Lan();
//				SetSystemPowerState(NULL, POWER_STATE_RESET,POWER_FORCE);
//				Sleep(2000);
//			}
//		}
//		//getip2
//		if(strcmp(&par[0][0],gi2)==0)
//		{
//			wchar_t mac[128];
//			mbstowcs(mac,&par[1][0] , strlen(&par[1][0])+1);//Plus null
//			//verifica che il comando sia per il mio MAC
//			if (wcscmp(WMAC , mac) == 0)
//			{
//				RETAILMSG(1, (TEXT("GETIP2:\r\n")));
//				SendGetIP2(0);
//			}
//		}
//		//getip2 unicast
//		if(strcmp(&par[0][0],ui2)==0)
//		{
//			wchar_t mac[128];
//			mbstowcs(mac,&par[1][0] , strlen(&par[1][0])+1);//Plus null
//			//verifica che il comando sia per il mio MAC
//			if (wcscmp(WMAC , mac) == 0)
//			{
//				RETAILMSG(1, (TEXT("GETIP2U:\r\n")));
//				SendGetIP2(1);
//			}
//		}
//
//
//		//logplc
//		if(strcmp(&par[0][0],log)==0)
//		{
//			if(CreateProcess(L"\\nandflash\\altraceprintce7.exe",L"llexec -fs \"/nandflash/logplc.txt\"",NULL,NULL,NULL,0,NULL,NULL,NULL,NULL))
//				RETAILMSG(1, (TEXT("LOGPLC.txt create\r\n")));
//		}
//
//		//START bridge lan to can 0
//		if(strcmp(&par[0][0],lc0)==0)
//		{			
//			if (KillProcess(L"LLEXECNOUI.EXE"))
//				RETAILMSG(1, (TEXT("KILL:llexecnoui.exe \r\n")));
//			if (KillProcess(L"LLEXEC.EXE"))
//				RETAILMSG(1, (TEXT("KILL:llexec.exe \r\n")));
//			SetBridgeLANToCAN(0,atoi(&par[1][0]));
//			WCHAR name[255] = {0};
//			swprintf_s(name,sizeof(name),L"RLAN>CAN0,%s",WMAC);
//			int size_utf16 = wcslen(name)*2;
//			strcpy(ipcan0bridge,inet_ntoa(SenderAddr.sin_addr));
//			TXinfo.sin_addr.s_addr = inet_addr(ipcan0bridge);		///< IP Address of sender
//			retVal = sendto(TXSocket,(char*)name,size_utf16, 0, (SOCKADDR *) & TXinfo, sizeof (TXinfo));
//			if (retVal == SOCKET_ERROR)
//			{
//				RETAILMSG(1, (TEXT("\nCould not send message to Server with error code : %d"), WSAGetLastError()));
//			}
//		}
//
//		//START bridge lan to can 1
//		if(strcmp(&par[0][0],lc1)==0)
//		{			
//			if (KillProcess(L"LLEXECNOUI.EXE"))
//				RETAILMSG(1, (TEXT("KILL:llexecnoui.exe \r\n")));
//			if (KillProcess(L"LLEXEC.EXE"))
//				RETAILMSG(1, (TEXT("KILL:llexec.exe \r\n")));
//			SetBridgeLANToCAN(1,atoi(&par[1][0]));
//			WCHAR name[255] = {0};
//			swprintf_s(name,sizeof(name),L"RLAN>CAN1,%s",WMAC);
//			int size_utf16 = wcslen(name)*2; 
//			strcpy(ipcan1bridge,inet_ntoa(SenderAddr.sin_addr));
//			TXinfo.sin_addr.s_addr = inet_addr(ipcan1bridge);		///< IP Address of sender
//			retVal = sendto(TXSocket,(char*)name,size_utf16, 0, (SOCKADDR *) & TXinfo, sizeof (TXinfo));
//			if (retVal == SOCKET_ERROR)
//			{
//				RETAILMSG(1, (TEXT("\nCould not send message to Server with error code : %d"), WSAGetLastError()));
//			}
//		}
//
//		//START bridge lan to COM2
//		if(strcmp(&par[0][0],lc1)==0)
//		{			
//			if (KillProcess(L"LLEXECNOUI.EXE"))
//				RETAILMSG(1, (TEXT("KILL:llexecnoui.exe \r\n")));
//			if (KillProcess(L"LLEXEC.EXE"))
//				RETAILMSG(1, (TEXT("KILL:llexec.exe \r\n")));
//			SetBridgeLANToCOM(2,atoi(&par[1][0]));
//			WCHAR name[255] = {0};
//			swprintf_s(name,sizeof(name),L"RLAN>COM2,%s",WMAC);
//			int size_utf16 = wcslen(name)*2; 
//			strcpy(ipcom2bridge,inet_ntoa(SenderAddr.sin_addr));
//			TXinfo.sin_addr.s_addr = inet_addr(ipcom2bridge);		///< IP Address of sender
//			retVal = sendto(TXSocket,(char*)name,size_utf16, 0, (SOCKADDR *) & TXinfo, sizeof (TXinfo));
//			if (retVal == SOCKET_ERROR)
//			{
//				RETAILMSG(1, (TEXT("\nCould not send message to Server with error code : %d"), WSAGetLastError()));
//			}
//		}

//	}	
//}