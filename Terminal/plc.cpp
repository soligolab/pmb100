#include <stdio.h>
#include <stdlib.h>
#include <errno.h> 
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/resource.h>
#include <ctype.h>
#include <time.h>

#include <terminale_led.h>
#include <led_net.h>
#include <plc.h>
#include <util.h>

pid_t pidplc;
bool plcON = false;
bool plcSTOP = false;
bool StopRuntime=false;

#define BINCODESYS  (char*)"codesyscontrol"
#define BINLOGICLAB (char*)"LLExec"

bool PidExist(pid_t pi)
{	
	struct stat sts;
	if ( pi == 0) return false;
	char  str[16];
	sprintf(str, "/proc/%d", pi);
	if (stat(str, &sts) == -1 && errno == ENOENT)
	{
		return false; 	// process doesn't exit
	}
	return true; 
}

pid_t GetPid(char * bin)
{
	FILE *cmd_pipe;
	char buf[512];
	char  str[64];
	sprintf(str, "pidof %s", bin);
	cmd_pipe = popen(str, "r");
	//cmd_pipe = popen("pidof codesyscontrol.bin", "r");
	fgets(buf, 512, cmd_pipe);
	pid_t pid = strtoul(buf, NULL, 10);
	pclose(cmd_pipe); 
	return pid;
}

pid_t Get_tmp_pid(char * file)
{
	int lenbuf = 8;
	char pid[lenbuf];
	int f = open(file, O_RDONLY);
	if (f == -1) return 0;
	read(f, pid, lenbuf);
	close(f);
	return atoi(pid);
}

pid_t GetPidPlc(void)
{
	if (TipoPlc == PLCCODESYS)
		return  GetPid(BINCODESYS);
	if (TipoPlc == PLCLOGICLAB)
		return GetPid(BINLOGICLAB);
	return 0;
}
void StartRuntimePLC(void)
{	
	if (PidExist(pidplc) == false)
	{
		if (TipoPlc == PLCCODESYS)
		{
			dmesgDebug("Start CODESYS", "");
			system("systemctl start codesyscontrol.service");
			sleep(1);
			pidplc = GetPid(BINCODESYS);
		}
		if (TipoPlc == PLCLOGICLAB)
		{
			dmesgDebug("Start LOGICLAB", "");
			system("systemctl start LLExecDaemon.service");
			sleep(1);
			pidplc = GetPid(BINLOGICLAB);
		}
	}
}	

void StopRuntimePLC(void)
{
	if (PidExist(pidplc) == true)
	{
		if (TipoPlc == PLCCODESYS)
		{
			dmesgDebug("Stop CODESYS", "");
			system("systemctl stop codesyscontrol.service;ip link set down can0;ip link set down can1");
			pidplc = GetPid(BINCODESYS);
		}
		if (TipoPlc == PLCLOGICLAB)
		{
			dmesgDebug("Stop LOGICLAB", "");
			system("systemctl stop LLExecDaemon.service;ip link set down can0;ip link set down can1");
			pidplc = GetPid(BINLOGICLAB);
		}
	}		
}

void Save_pidplc (void)
{
	int fd;
	char buffer[10];
	sprintf(buffer,"%d",pidplc);
	fd = open("/tmp/pidplc", O_CREAT | O_RDWR);
	write(fd, buffer,strlen(buffer));
	close(fd);
}

int GetStateInFile(const char *file ,char defstate)
{
	FILE * fd = fopen(file,"r");
	if (fd == NULL) return defstate;
	int c = getc(fd);
	fclose(fd);
	return c;
}

static int MakeFile(const char *filename, const char * strin) 
{
	int fd = open(filename, O_CREAT | O_RDWR);
	if (fd == -1) return 0;
	write(fd, strin, 1);
	close(fd);
	return 1;
}

bool floneshot = false;
int stopdipold=-1;
void GestionePLC(void)
{
	char state;
	
	//gestione stato runtime e stato logica all'avvio
	if (floneshot == false)
	{	
		floneshot = true;
		//state = GetStateInFile("/data/startstop.cmd",'0');
		//if (state == '0')  MakeFile("/tmp/startstop.cmd", "0");
		//if (state == '1')  MakeFile("/tmp/startstop.cmd", "1");
		//start stop runtime
		state = GetStateInFile("/data/runtimestartstop.cmd", '0');
		if (state == '0')  MakeFile("/tmp/runtimestartstop.cmd", "0");
		if (state == '1')  MakeFile("/tmp/runtimestartstop.cmd", "1");
	}
	
	//gestione start stop logica PLC da dip
	int stopdip = (Dip & BIT_DIP1_STOP);
	if (stopdip != stopdipold) 
	{
		if (stopdip == 1) 
		{
			MakeFile("/tmp/startstop.cmd", "1");
		}
		if (stopdip == 0)
		{	
			MakeFile("/tmp/startstop.cmd", "0");
		}
		stopdipold = stopdip;
	}
	//solo per LOGICLAB
	if (TipoPlc == PLCLOGICLAB)
	{
		MakeFile("/tmp/runapp.state", "1");
		if (stopdip == 1)  MakeFile("/tmp/runtimestartstop.cmd", "1");
		if (stopdip == 0)  MakeFile("/tmp/runtimestartstop.cmd", "0");
	}
	
	
	//gestione start stop Runtime da file
	state = GetStateInFile("/tmp/runtimestartstop.cmd",'0');
	if (state == '0') StopRuntime = false; 
	if (state == '1') StopRuntime = true; 
		
	//avvia runtime
	if ((StopRuntime == false) && (plcON == false))
	{
		LedTRMsup |= LED_RUN; //0x10000; //on led RUN
		LedTRMinf &= ~LED_STOP; //0x100; //off led stop
		LedBLKinf &= ~LED_STOP; //0x100; //off led blink stop
		StartRuntimePLC(); //start solo se pidplc non esiste
		plcON = true;
		plcSTOP = false;
		Save_pidplc();
	}
	
	//kill runtime
	if ((StopRuntime == true) && (plcSTOP == false))
	{
		LedTRMinf |=  LED_STOP; //0x100; //on led STOP
		LedTRMsup &= ~LED_RUN; //0x10000; //off led RUN
		LedTRMinf &= ~LED_E_1; //0x1000; //off E1
		LedBLKinf &= ~LED_E_1; //0x1000; //off lamp
		StopRuntimePLC(); //stop solo se pidplc esiste
		plcSTOP = true;
		plcON = false;
		remove("/tmp/pidplc");
	}
	
	//led start e stop
	if (PidExist(pidplc))  //plc Is Run
	{
		LedTRMsup |= LED_RUN;//0x10000; //on led RUN
		LedBLKsup &= ~LED_RUN;//0x10000; //off lampeggio
		//gestione stop Logico
		if (GetStateInFile("/tmp/runapp.state",'0') == '1') //0=stop 1=RUN
		{
			LedTRMinf &= ~LED_STOP;//0x100;  //off led stop
			LedBLKinf &= ~LED_STOP;
		}	
		else
		{
			LedTRMinf |= LED_STOP; //0x100;  //on led stop
			LedBLKinf  |= LED_STOP;
		}	
	}
	else
	{
		LedTRMinf |= LED_STOP; //0x100;    //on led STOP
		LedBLKinf &= ~LED_STOP; //0x100;   //off led blink stop
		LedTRMsup &= ~LED_RUN; //0x10000; //off led RUN
		fcan0error = false;
		fcan1error = false;
		//se 
		if (plcON == true)
		{
			msleep(2500); //ritardo per riavvio PLC
			//pidplc = GetPidPlc();
			plcON = false;         
			plcSTOP = false;
			LedTRMinf |= LED_E_1;//0x1000; //on E1
			LedBLKinf |= LED_E_1;//0x1000; //on lamp
			dmesgDebug("!!! Restart PLC !!!", "");
		}
	}	
}

