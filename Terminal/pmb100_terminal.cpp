#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/resource.h>
#include <linux/reboot.h> 
#include <sys/reboot.h> 
#include <sys/syscall.h>

#include "terminale_led.h"
#include "candef.h"
#include "plc.h"
#include "servizio_device_finder.h"
#include "led_seriali.h"
#include "led_net.h"
#include "util.h"
#include <sys/stat.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>


extern struct ETHX ETHx[5];
extern pid_t pidplc;
char * BinName;
int LifeTimeMMC_1to10;
int RunDisk;
int TempoPressioneS1=0;
pid_t pidcopy2emmc;
int RitardoStartPLCSeNonPresenteTRM = 0;
int TipoBoard = NOBOARD;
int TipoPlc;
//char StrTipoPlc[32] = {0};
char  SerialNumber[32] = { 0 };
char  Device_Name[32] = { 0 };
bool PlPowerOff = 0;
bool LicPresent = true;
bool	blampallledinf;
bool	blampallledsup;
bool	blampallled;

int Modalita = MODALITA_RUN;

bool CiclaSuWhile = true;

extern bool StopRuntime;

#ifdef UPS
/* This is the critical section object (statically allocated). */
static pthread_mutex_t cs_mutex =  PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

void ApplicoIP99(void)
{
	system("nmcli connection mod eth0 ipv4.method manual ipv4.addresses 192.168.0.99/24 ipv4.gateway 0.0.0.0 ipv4.dns \"0.0.0.0 0.0.0.0\"");
	system("nmcli device reapply eth0"); //applica subito le modifiche
}

bool emmslcok = false;
bool copy2emmc = false;
bool fls1prem = false;
void GestionePulsanteS1_collaudo_RUNMMC(void)
{	
	//*************************************************** prepara PLC per collaudo o per spedizione
	if (Dip & BIT_PULSANTE ) 
	{
		TempoPressioneS1++;
		blampallledinf = false;
		fls1prem = true;
	}
	else
	{
		//Lampeggio led in fase di collaudo
		if (fls1prem == false)
		{
			if (Dip == 0x4)
			{	
				blampallledsup = true;
				blampallledinf = false;
			}
			else
			{	
				blampallledsup = false;
				blampallledinf = true;
			}
		}
		//gestione fine collaudo rimozione di tutti i file di collaudo per spedizione
		if ((TempoPressioneS1 >= 10) && (TipoPlc != NOCONF))
		{
			if (Dip == 0x4 && FileExist("/data/finalized") == true && FileExist("/data/collaudook") == true)  //dip tutti a 0 tranne NFC e PLC battezzato da DeviceFinder e test collaudo OK
			{
				//Prepara PLC per spedizione
				TempoPressioneS1 = 0;
				blampallledinf = false;
				//stop runtime
				system("systemctl stop codesyscontrol");
				if (TipoPlc == PLCCODESYS)
				{
					//cancella programma di test
					system("rm /data/codesys/PlcLogic/Application/*");
					system("rm /data/codesys/PlcLogic/visu/*");
					system("rm -r /data/codesys/.pki/*");
					remove("/data/codesys/SysFileMap.cfg");
					//
					system("rm -r /data/plc/*");
					system("rmdir /data/plc"); 
					//copio 3s.dat e tutto ciò che trovo se sono un PL700
					if (strcmp(Device_Name, "PL700-340-1AD") == 0) system("cp -r /data/codesys/PL700-340-1AD/* /data/codesys"); 
					if (strcmp(Device_Name, "PL700-340-2AD") == 0) system("cp -r /data/codesys/PL700-340-2AD/* /data/codesys"); 
					if (strcmp(Device_Name, "PL700-340-3AD") == 0) system("cp -r /data/codesys/PL700-340-3AD/* /data/codesys"); 
					if (strcmp(Device_Name, "PL700-340-4AD") == 0) system("cp -r /data/codesys/PL700-340-4AD/* /data/codesys");
					if (strcmp(Device_Name, "PL700-340-5AD") == 0) system("cp -r /data/codesys/PL700-340-5AD/* /data/codesys");
					if (strcmp(Device_Name, "PL700-340-6AD") == 0) system("cp -r /data/codesys/PL700-340-6AD/* /data/codesys");
					//proteggo come root il file delle licenze
					system("chmod 600 /data/codesys/3S.dat");
					//cancello tutto ciò che non serve piu
					system("rm -r /data/codesys/PL700-340-1AD/*"); 
					system("rm -r /data/codesys/PL700-340-2AD/*"); 
					system("rm -r /data/codesys/PL700-340-3AD/*"); 
					system("rm -r /data/codesys/PL700-340-4AD/*");
					system("rm -r /data/codesys/PL700-340-5AD/*");
					system("rm -r /data/codesys/PL700-340-6AD/*");
					//cancello DIR
					system("rmdir /data/codesys/PL700-340-1AD"); 
					system("rmdir /data/codesys/PL700-340-2AD"); 
					system("rmdir /data/codesys/PL700-340-3AD"); 
					system("rmdir /data/codesys/PL700-340-4AD");
					system("rmdir /data/codesys/PL700-340-5AD");
					system("rmdir /data/codesys/PL700-340-6AD");
				}
				else if (TipoPlc == PLCLOGICLAB)
				{	
					system("rm -r /data/codesys/*");
					system("rmdir /data/codesys"); 
					system("rm -r /data/edge/*");
					system("rmdir /data/edge");
					remove("/etc/systemd/system/codesysedge.service");
				}
				remove("/data/ups.txt");
				//applico Default IP 192.168.0.99
				system("nmcli connection delete eth0");
				system("nmcli connection add con-name eth0 ifname eth0 type ethernet");
				ApplicoIP99();
				
				
				//system("nmcli connection mod eth0 ipv4.method manual ipv4.addresses 192.168.0.99/24 ipv4.gateway 0.0.0.0 ipv4.dns \"0.0.0.0 0.0.0.0\"");
				//system("nmcli device mod eth0 ipv4.method manual ipv4.addresses 192.168.0.99/24 ipv4.gateway 0.0.0.0 ipv4.dns \"0.0.0.0 0.0.0.0\"");
				
				//nmcli connection modify eth0 ipv4.method manual ipv4.addresses 192.168.0.99/24 connection.autoconnect yes
				//nmcli device modify eth0 ipv4.method $ ipv4_method ipv4.addresses $ ipv4_addresses ipv4.gateway $ netgw ipv4.dns $ netdns connection.autoconnect yes

				//reboot
				remove("/data/collaudo");
				remove("/data/finalized");
				remove("/data/collaudook");
				remove("/data/disablecanerror");
				system("reboot");
				system("systemctl stop pmb100-terminal");
				msleep(4000);
				return;
			}
		}
				
		//setta LAN0 in DHCP se manca file eth0.nmconnection
		if (FileExist("/etc/NetworkManager/system-connections/eth0.nmconnection") == false)
		{
			//creo il file eth0.connection se non presente
			system("nmcli connection add con-name eth0 ifname eth0 type ethernet");
			//applico new IP
			system("nmcli connection mod eth0 ipv4.method auto");
			system("nmcli device reapply eth0"); //applica subito le modifiche
		}
	}
}


void PreparaEmmc_Sd_PerCODESYS()
{
	//cancella programma di test
	system("rm /data/codesys/PlcLogic/Application/*");
	system("rm /data/codesys/PlcLogic/visu/*");
	system("rm -r /data/codesys/.pki/*");
	remove("/data/codesys/SysFileMap.cfg");
	//
	system("rm -r /data/plc/*");
	system("rmdir /data/plc"); 
	//copio 3s.dat e tutto ciò che trovo se sono un PL700
	if (strcmp(Device_Name, "PL700-340-1AD") == 0) system("cp -r /data/codesys/PL700-340-1AD/* /data/codesys"); 
	if (strcmp(Device_Name, "PL700-340-2AD") == 0) system("cp -r /data/codesys/PL700-340-2AD/* /data/codesys"); 
	if (strcmp(Device_Name, "PL700-340-3AD") == 0) system("cp -r /data/codesys/PL700-340-3AD/* /data/codesys"); 
	if (strcmp(Device_Name, "PL700-340-4AD") == 0) system("cp -r /data/codesys/PL700-340-4AD/* /data/codesys");
	if (strcmp(Device_Name, "PL700-340-5AD") == 0) system("cp -r /data/codesys/PL700-340-5AD/* /data/codesys");
	if (strcmp(Device_Name, "PL700-340-6AD") == 0) system("cp -r /data/codesys/PL700-340-6AD/* /data/codesys");
	//decripto licenza 3s.dat
	system("openssl enc -aes-256-cbc -pbkdf2 -d -salt -in /data/codesys/3S.dat.enc -out /data/codesys/3S.dat -k 12345689abcdefghi");
	remove("/data/codesys/3S.dat.enc");
	//proteggo come root il file delle licenze
	system("chmod 600 /data/codesys/3S.dat");
	//cancello tutto ciò che non serve piu
	system("rm -r /data/codesys/PL700-340-1AD/*"); 
	system("rm -r /data/codesys/PL700-340-2AD/*"); 
	system("rm -r /data/codesys/PL700-340-3AD/*"); 
	system("rm -r /data/codesys/PL700-340-4AD/*");
	system("rm -r /data/codesys/PL700-340-5AD/*");
	system("rm -r /data/codesys/PL700-340-6AD/*");
	//cancello DIR
	system("rmdir /data/codesys/PL700-340-1AD"); 
	system("rmdir /data/codesys/PL700-340-2AD"); 
	system("rmdir /data/codesys/PL700-340-3AD"); 
	system("rmdir /data/codesys/PL700-340-4AD");
	system("rmdir /data/codesys/PL700-340-5AD");
	system("rmdir /data/codesys/PL700-340-6AD");
}


void PreparaEmmcPerPPH()
{	
	//Vlan1
	system("nmcli connection add type vlan con-name eth0.1 ifname eth0.1 dev eth0 id 1");
	system("nmcli connection modify eth0.1 ipv4.method auto");
	//Vlan2
	system("nmcli connection add type vlan con-name eth0.2 ifname eth0.2 dev eth0 id 2");
	system("nmcli connection modify eth0.2 ipv4.addresses 192.168.10.1/24");
	system("nmcli connection modify eth0.2 ipv4.method manual");
	system("nmcli connection up eth0.1");
	system("nmcli connection up eth0.2");
	//
	//system("nmcli connection down eth0");
	//remove("/etc/NetworkManager/system-connections/eth0.nmconnection");
	//del file
	remove("/data/finalized");
	remove("/data/collaudo");
	system("rm -rf /data/plc/*");
	system("rm -rf /data/codesys/*");
	system("rmdir /data/plc"); 
	system("rmdir /data/codesys");
}	
	
void GestionePulsanteS1_collaudo_RUNSD(void)
{	
	if (Dip & BIT_PULSANTE) 
	{
		TempoPressioneS1++;
	}
	else
	{
		//esci se copia è stata lanciata
		if (copy2emmc == true) return;
		
		//se EMMC format in SLC la prima volta , lancia copia in automatico senza premere pulsante
		if(FileExist("/data/emmcslc.ok") == true) emmslcok = true;

		// Tempo > di 2 sec. e DIP tutti a 1 tranne NFC per copiare SD in EMMC 
		if (((TempoPressioneS1 >= 20) && (Dip == 0xB)) || emmslcok == true) 
		{
			//cancella eventuali file da non copiare
			remove("/data/collaudook");
			remove("/data/finalized");
			remove("/data/poweroff.txt");
			remove("/data/codesyscontrol.log");
			
			TempoPressioneS1 = 0;
			copy2emmc = true;
			LedTRMinf |= LED_INTERNO  | LED_E_1 | LED_E_2 | LED_W_1 | LED_DISCO; //
			LedBLKinf |=  LED_INTERNO | LED_E_1 | LED_E_2 | LED_W_1 | LED_DISCO; //
			remove("/tmp/sh.ok");
			
			//se EMMC non formattata SLC: fornatta e crea file "/data/emmcslc.ok" che serve per partenza automatica al riavvio dopo power off
			//se EMMC è gia formattata procede subito con la copia 
			//In fine copia se tutto ok , genera file sh.ok
			system("/opt/backup/uboot/copy2emmc.sh ext4 nopersistent");
			if (FileExist("/tmp/sh.ok") == true)
			{
				LedBLKinf &= ~(LED_INTERNO | LED_E_1 | LED_E_2 | LED_W_1 | LED_DISCO); //se ok stop lampeggio
				//fine copia
				if (emmslcok == true) remove("/data/emmcslc.ok");
			}
			else
			{
				//lampeggio all per ERRORE
				blampallled = true;
				LedTRMinf &= ~(LED_INTERNO | LED_E_1 | LED_E_2 | LED_W_1 | LED_DISCO); //se ok stop led
				LedBLKinf &= ~(LED_INTERNO | LED_E_1 | LED_E_2 | LED_W_1 | LED_DISCO); //se ok stop lampeggio
			}
			emmslcok = false;
			sync();
		}
	}
}
	
void GestionePulsanteS1(void)
{
	if (Dip & BIT_PULSANTE ) 
	{
		TempoPressioneS1++;
		
		//Pulsante S1 ferma lampeggio Led se Licenza non presente
		if (TipoPlc == PLCCODESYS)
		{
			if (LicPresent == false)
			{
				LicPresent = true;
				blampallled = false;
			}
			blampallledinf = false;
		}
	}
	else
	{	
		//Run in EMMC
		if (RunDisk == RUNMMC)
		{	
			//gestione copia del LOG PLC su USB se presente
			if ((TempoPressioneS1 >= 15) && (TempoPressioneS1 <= 50))
			{
				TempoPressioneS1 = 0;
				LedTRMinf |= LED_INTERNO;  //on lampeggiante
				LedBLKinf |= LED_INTERNO;
				remove("/tmp/sh.ok");
				StopRuntimePLC();
				if (TipoPlc == PLCCODESYS)  system("/data/pmb100/createlog.sh");
				if (TipoPlc == PLCLOGICLAB) system("/data/pmb100/createlog.sh");
				if (TipoPlc == CNV600)      system("/data/pmb100/createlog.sh");
				//tutto ok se presente file sh.ok
				if (FileExist("/tmp/sh.ok") == true)
				{
					LedTRMinf &= ~LED_INTERNO;
					LedBLKinf &= ~LED_INTERNO;  //se ok stop lampeggio
					StartRuntimePLC();
				}
				else
					blampallled=true; //se ko lampeggia tutti i led
			}
			//Gestione copia backup DA USB in EMMC 
			if (TempoPressioneS1 >= 80  && (TempoPressioneS1 <= 180))
			{
				TempoPressioneS1 = 0;
				LedTRMinf |= LED_INTERNO;  //on lampeggiante
				LedBLKinf |= LED_INTERNO;
				remove("/tmp/sh.ok");
				StopRuntimePLC();
				if (TipoPlc == PLCCODESYS)   system("/data/pmb100/restore.sh");
			    if (TipoPlc == PLCLOGICLAB)  system("/data/pmb100/restore.sh");
				if (TipoPlc == CNV600)		 system("/data/pmb100/restore.sh");
				//tutto ok se presente file sh.ok
				if (FileExist("/tmp/sh.ok") == true)
				{
					LedTRMinf &= ~LED_INTERNO;
					LedBLKinf &= ~LED_INTERNO; //se ok stop lampeggio
					StartRuntimePLC();
				}
				else
				{
					LedBLKinf &= ~LED_INTERNO; //stop lampeggio
					blampallled = true; //se ko lampeggia tutti i led
				}
			}
			if (TempoPressioneS1 >= 240)
			{
				system("systemctl stop pmb100-terminal.service"); //stop servizio LED
				StartDebug_tty0();
				CiclaSuWhile = false;
			}
				
		}
		//Run in SD
		if (RunDisk == RUNSD)
		{	
			// Tempo > di 2 sec. e DIP stop a 1 per copiare SD in EMMC per upgrade (manca il file collaudo)
			if ((TempoPressioneS1 >= 20) && (Dip & BIT_DIP1_STOP) ) 
			{
				TempoPressioneS1 = 0;
				LedTRMinf |= LED_INTERNO  | LED_E_1 | LED_E_2 | LED_W_1 | LED_DISCO; //
				LedBLKinf |=  LED_INTERNO | LED_E_1 | LED_E_2 | LED_W_1 | LED_DISCO; //
				remove("/tmp/sh.ok");
				system("/opt/backup/uboot/copy2emmc.sh ext4 persistent");
				//tutto ok se presente file sh.ok
				if (FileExist("/tmp/sh.ok") == true)
				{
					LedBLKinf &= ~(LED_INTERNO | LED_E_1 | LED_E_2 | LED_W_1 | LED_DISCO); //se ok stop lampeggio
				}
				else
				{
					blampallled = true;
					LedTRMinf &= ~(LED_INTERNO | LED_E_1 | LED_E_2 | LED_W_1 | LED_DISCO); //se ok stop led
					LedBLKinf &= ~(LED_INTERNO | LED_E_1 | LED_E_2 | LED_W_1 | LED_DISCO); //se ok stop lampeggio
				}
			}
		}
		TempoPressioneS1 = 0;
	}	
}

bool DIP99on=false;
bool DIP99off = false;
void GestioneDipIp99(void)
{
	char str[64] = { 0 };
	if (Dip & BIT_DIP2_192_168_0_99)
	{
		//dip ON
		if (DIP99on == true) return;
		DIP99on = true;
		DIP99off = false;
		if (FileExist("/etc/NetworkManager/system-connections/eth0.nmconnection") == false)
		{	//creo il file eth0.connection se non presente
			system("nmcli connection add con-name eth0 ifname eth0 type ethernet");
			//applico new IP
			ApplicoIP99();
		}
		else
		{	//se presente file eth0.connection verifico che sia presente IP
			if (CercaStrSuFile((char*)"/etc/NetworkManager/system-connections/eth0.nmconnection", (char*)"address1=", str) == false) 
			{	//se IP non presente cancello connesione (potrebbe essere bridge) e creo new etho
				system("nmcli connection delete eth0");
				system("nmcli connection add con-name eth0 ifname eth0 type ethernet");
				//applico new IP
				ApplicoIP99();
			}
			else
			{  //se IP presente verifico sia uguale a "192.168.0.99/24,192.168.0.254"
				if (strcmp(str, "address1=192.168.0.99/24,0.0.0.0") != 0)
				{	//se diverso applico new IP
					ApplicoIP99();
				}
			}	
		}	
	}
	else
	{	//dip OFF
		DIP99on = false;
		if (DIP99off == true) return;
		DIP99off = true;
		if (FileExist("/etc/NetworkManager/system-connections/eth0.nmconnection") == false)
		{
			//creo il file eth0.connection se non presente
			system("nmcli connection add con-name eth0 ifname eth0 type ethernet");
			//applico new IP
			system("nmcli connection mod eth0 ipv4.method auto");
			system("nmcli device reapply eth0"); //applica subito le modifiche
		}
	}	
}	

bool ReadParNFC = false;
bool ReadParNFCok = false;
bool DipNfcon = false;
bool DipNfoff = false;

void GestioneDipNFC()
{
	if (Dip & BIT_DIP3_NFC)
	{
		//Dip ON
		if (ReadParNFCok==true)
		{	
			DipNfcon = false;
			ReadParNFCok=false;
			ApplicaParametriNFC();
		}	
		if (DipNfcon == true) return;
		if (Dip & BIT_NFC_MODIFY)
		{	
			DipNfcon = true;
			ReadParNFC=true;
		}	
	}
	else
	{
		//Dip OFF
		if (DipNfoff == true) return;
		DipNfoff = true;
		DipNfcon = false;
	
		
	}	
}	
	
bool DIPvpnrun = false;
bool InitVPN = false;
int TimerRitRunVPN=0;
void GestioneDipVPN(void)
{
	if (TimerRitRunVPN >= 350)  //20 secondi
	{
		if (Dip & BIT_DIP4_VPN)
		{
			//dip ON
			if (DIPvpnrun == true)
			{
				LedTRMsup |= LED_VPN;
				return;
			}
			DIPvpnrun = true;
			system("systemctl start pixsysportal.service");
		}
		else
		{
			//dip OFF
			if (DIPvpnrun == true)
			{
				DIPvpnrun = false;
				system("systemctl stop pixsysportal.service");
				LedTRMsup &= ~LED_VPN;
			}		
		}		
	}
	else
	{
		TimerRitRunVPN++;	
	}
}	

#include <map>
#if defined(__has_include)
#if __has_include(<gpiod.h>)
#define PMB100_HAS_GPIOD 1
#include <gpiod.h>
#endif
#endif
#ifndef PMB100_HAS_GPIOD
#define PMB100_HAS_GPIOD 0
#endif

static bool use_sysfs_gpio = true;
static std::map<unsigned int, bool> gpio_direction_out;

static const char* gpio_label_from_legacy_number(unsigned int gpio)
{
	switch (gpio)
	{
		case 67: return "UPS_VMON";
		case 114: return "UPS_EN";
		case 68: return "ID_CAN";
		case 116: return "TP4";
		case 115: return "TP21";
		case 61: return "TP23";
		case 65: return "TP24";
		case 66: return "HWID2";
		case 69: return "HWID1";
		case 60: return "HWID0";
		case 53: return "USR0-TP11";
		case 54: return "USR1-TP12";
		case 55: return "USR2-TP13";
		case 56: return "USR3-TP14";
		default: return nullptr;
	}
}

static bool gpio_path_to_number(const char* path, unsigned int& gpio)
{
	if (path == nullptr) return false;
	return sscanf(path, "/sys/class/gpio/gpio%u/", &gpio) == 1;
}

#if PMB100_HAS_GPIOD
static int gpiod_find_offset_by_name(const char* name, const char* chip_path, unsigned int* offset)
{
	struct gpiod_chip* chip = gpiod_chip_open(chip_path);
	if (!chip) return -1;
	int rc = gpiod_chip_get_line_offset_from_name(chip, name);
	gpiod_chip_close(chip);
	if (rc < 0) return -1;
	*offset = (unsigned int)rc;
	return 0;
}

static int gpiod_resolve_offset(unsigned int gpio, const char** chip_path, unsigned int* offset)
{
	const char* name = gpio_label_from_legacy_number(gpio);
	if (!name) return -1;
	if (gpiod_find_offset_by_name(name, "/dev/gpiochip0", offset) == 0)
	{
		*chip_path = "/dev/gpiochip0";
		return 0;
	}
	if (gpiod_find_offset_by_name(name, "/dev/gpiochip1", offset) == 0)
	{
		*chip_path = "/dev/gpiochip1";
		return 0;
	}
	return -1;
}

static int gpiod_request_line_value(unsigned int gpio, bool output, int set_value, int* read_value)
{
	const char* chip_path = nullptr;
	unsigned int offset = 0;
	if (gpiod_resolve_offset(gpio, &chip_path, &offset) < 0) return -1;

	struct gpiod_chip* chip = gpiod_chip_open(chip_path);
	if (!chip) return -1;
	struct gpiod_line_settings* settings = gpiod_line_settings_new();
	struct gpiod_line_config* line_cfg = gpiod_line_config_new();
	struct gpiod_request_config* req_cfg = gpiod_request_config_new();
	if (!settings || !line_cfg || !req_cfg)
	{
		gpiod_line_settings_free(settings);
		gpiod_line_config_free(line_cfg);
		gpiod_request_config_free(req_cfg);
		gpiod_chip_close(chip);
		return -1;
	}

	gpiod_request_config_set_consumer(req_cfg, "PMB100Terminal");
	gpiod_line_settings_set_direction(settings, output ? GPIOD_LINE_DIRECTION_OUTPUT : GPIOD_LINE_DIRECTION_INPUT);
	if (output)
		gpiod_line_settings_set_output_value(settings, set_value ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);

	if (gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings) < 0)
	{
		gpiod_line_settings_free(settings);
		gpiod_line_config_free(line_cfg);
		gpiod_request_config_free(req_cfg);
		gpiod_chip_close(chip);
		return -1;
	}

	struct gpiod_line_request* req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
	if (!req)
	{
		gpiod_line_settings_free(settings);
		gpiod_line_config_free(line_cfg);
		gpiod_request_config_free(req_cfg);
		gpiod_chip_close(chip);
		return -1;
	}

	int rc = 0;
	if (read_value)
	{
		int val = gpiod_line_request_get_value(req, offset);
		if (val < 0) rc = -1;
		else *read_value = (val == GPIOD_LINE_VALUE_ACTIVE) ? 1 : 0;
	}

	gpiod_line_request_release(req);
	gpiod_line_settings_free(settings);
	gpiod_line_config_free(line_cfg);
	gpiod_request_config_free(req_cfg);
	gpiod_chip_close(chip);
	return rc;
}
#endif

void gpio_export(unsigned int gpio)
{
#if PMB100_HAS_GPIOD
	const char* label = gpio_label_from_legacy_number(gpio);
	if (label != nullptr)
	{
		unsigned int offset = 0;
		const char* chip_path = nullptr;
		if (gpiod_resolve_offset(gpio, &chip_path, &offset) == 0)
		{
			use_sysfs_gpio = false;
			return;
		}
	}
#endif
	FILE *f = fopen("/sys/class/gpio/export", "w");
	if (!f) return;
	fseek(f, 0, SEEK_SET);
	fprintf(f, "%d", gpio);
	fflush(f);
	fclose(f);
}

void gpio_unexport(unsigned int gpio)
{
	if (!use_sysfs_gpio) return;
	FILE *f = fopen("/sys/class/gpio/unexport", "w");
	if (!f) return;
	fseek(f, 0, SEEK_SET);
	fprintf(f, "%d", gpio);
	fflush(f);
	fclose(f);
}

void gpio_set_dir(const char*  Pathgpio, const char* type)
{
	unsigned int gpio = 0;
	if (gpio_path_to_number(Pathgpio, gpio))
		gpio_direction_out[gpio] = (strcmp(type, "out") == 0);
	if (!use_sysfs_gpio) return;
	FILE *f = fopen(Pathgpio, "w");
	if (!f) return;
	fseek(f, 0, SEEK_SET);
	fprintf(f, "%s", type);
	fflush(f);
	fclose(f);
}

int gpio_get_value(const char*  Pathgpio)
{
	unsigned int gpio = 0;
	if (gpio_path_to_number(Pathgpio, gpio))
	{
#if PMB100_HAS_GPIOD
		if (!use_sysfs_gpio)
		{
			int val = 0;
			if (gpiod_request_line_value(gpio, false, 0, &val) == 0) return val;
		}
#endif
	}
	FILE *f = fopen(Pathgpio, "r");
	if (!f) return 0;
	char value[4] = {0};
	fread(value, 1, 3, f);
	fclose(f);
	return atoi(value);
}

void gpio_set_value(const char*  Pathgpio, const char* value)
{
	unsigned int gpio = 0;
	if (gpio_path_to_number(Pathgpio, gpio))
	{
#if PMB100_HAS_GPIOD
		if (!use_sysfs_gpio)
		{
			gpiod_request_line_value(gpio, true, atoi(value), nullptr);
			return;
		}
#endif
	}
	FILE *f = fopen(Pathgpio, "w");
	if (!f) return;
	fseek(f, 0, SEEK_SET);
	fprintf(f, "%s", value);
	fflush(f);
	fclose(f);
}

void LeggiFileInfo()
{
	dmesgDebug("LeggiFileInfo()", "");	
	//Leggi il tipo di plc
	TipoPlc = NOCONF;
	GetGrupParSubParFromFile("/etc/pixsys/device.info", "[Pixsys]", "Serial number", SerialNumber);
	GetGrupParSubParFromFile("/etc/pixsys/device.info", "[Pixsys]", "Device_Name", Device_Name);

		//PLC è stato marchiato con un Codice
		if (TipoBoard == BOARD335)
		{
			if (strcmp(Device_Name, "PL700-335-1AD") == 0) { TipoPlc = PLCCODESYS; }
			if (strcmp(Device_Name, "PL600-335-1AD") == 0) { TipoPlc = PLCLOGICLAB; }
			if (strcmp(Device_Name, "CNV600-335-1AD") == 0) { TipoPlc = CNV600; }
		}	
		if (TipoBoard == BOARD340)
		{
			if (strcmp(Device_Name, "PL700-340-1AD") == 0) { TipoPlc = PLCCODESYS; }
			if (strcmp(Device_Name, "PL700-340-2AD")  == 0) { TipoPlc = PLCCODESYS; }
			if (strcmp(Device_Name, "PL700-340-3AD")  == 0) { TipoPlc = PLCCODESYS; }
			if (strcmp(Device_Name, "PL700-340-4AD")  == 0) { TipoPlc = PLCCODESYS; }
			if (strcmp(Device_Name, "PL700-340-5AD")  == 0) { TipoPlc = PLCCODESYS; }
			if (strcmp(Device_Name, "PL600-340-1AD") == 0) { TipoPlc = PLCLOGICLAB; }
			if (strcmp(Device_Name, "CNV600-340-1AD") == 0) { TipoPlc = CNV600; }
		}
		if (TipoBoard == BOARDPPH)
		{
			if (strcmp(Device_Name, "PPH600-31A") == 0) { TipoPlc = PPH600; }	
			if (strcmp(Device_Name, "PPH600-61A") == 0) { TipoPlc = PPH600; }
		}
		
		
		char hostname[32];
		gethostname(hostname, 32);
		if ((strcmp(hostname, "pmb100") == 0) || (strcmp(hostname, "pphsd") == 0)  || (strcmp(hostname, "plsd") == 0))
		{ 
			//copia Device_Name su  hostname al passagio tra NOCONF e TipoPLC se hostname è uno di quelli sopra
			if (TipoPlc != NOCONF)
			{
				char cmd[64] = { 0 };
				char loc[64] = { 0 };
				//tutto minuscolo
				for (int i = 0; Device_Name[i] != '\0'; i++)
				{
					loc[i] = tolower(Device_Name[i]);
				}
				//set hostname
				strcat(cmd, "hostnamectl set-hostname '"); strcat(cmd, loc); strcat(cmd, "'"); system(cmd);
				//set pretty
				cmd[0] = 0;
				strcat(cmd, "hostnamectl set-hostname '"); strcat(cmd, loc); strcat(cmd, "'"); strcat(cmd, " --pretty"); system(cmd);
			}
		}
	
	if (TipoPlc   == NOCONF)
	{	
		dmesgDebug("ERROR NOCONF DEVICE", "");	
		blampallledsup = true;
	}
	if (TipoPlc == PLCLOGICLAB)
	{	
		dmesgDebug("PL600 LogicLab", "");	
	}
	if (TipoPlc == PLCCODESYS)
	{
		dmesgDebug("PL700 Codesys", "");	
	}
	if (TipoPlc == CNV600)
	{
		dmesgDebug("CNV600 Gateway VPN", "");
		LedTRMsup = LED_RUN; // led run acceso
	}
	if (TipoPlc == PPH600)
	{
		dmesgDebug("PPH600 Gateway VPN", "");
		LedTRMsup = LED_RUN; // led run acceso
	}
}

void InizializzaGPIO(void)
{
// activate sysfs interface GPIO  (R7,T6,U18)open1=335 (R7,T6,U18)close0=340 
	
	dmesgDebug("InizializzaGPIO()", "");	
	gpio_unexport(60);
	gpio_export(60);
	gpio_set_dir("/sys/class/gpio/gpio60/direction", "in");
	int U18 = gpio_get_value("/sys/class/gpio/gpio60/value");
	
	//R7=66 =0 per hw_340
	gpio_unexport(66);
	gpio_export(66);
	gpio_set_dir("/sys/class/gpio/gpio66/direction", "in");
	int R7  = gpio_get_value("/sys/class/gpio/gpio66/value");
	
	// T6=69 =0 per hw_340
	gpio_unexport(69);
	gpio_export(69);
	gpio_set_dir("/sys/class/gpio/gpio69/direction", "in");
	int T6  = gpio_get_value("/sys/class/gpio/gpio69/value");
	
	//T7=67 (VMON) UPS per SHUTDOWN 1= ON
	gpio_unexport(67);
	gpio_export(67);
	gpio_set_dir("/sys/class/gpio/gpio67/direction", "in");
	int T7  = gpio_get_value("/sys/class/gpio/gpio67/value");	
	
	//B12=114  UPS Enable
	gpio_unexport(114);
	gpio_export(114);
	gpio_set_dir("/sys/class/gpio/gpio114/direction", "out");
	gpio_set_value("/sys/class/gpio/gpio114/value", "0");	
	
	//U6=68 (IDCAN) numerazione CAN0 =1 abilita primo PLE
	gpio_unexport(68);
	gpio_export(68);
	gpio_set_dir("/sys/class/gpio/gpio68/direction", "out");
	gpio_set_value("/sys/class/gpio/gpio68/value", "1");	
	
	if ((R7 && T6 && U18) == 1)
		TipoBoard = BOARD335;
	if ((R7 && T6 && U18) == 0)
		TipoBoard = BOARD340;
	if (((R7 && T6 ) == 0) && (U18==1))
		TipoBoard = BOARDPPH;
	//loop stop per riavvio con UPS
	while(T7==0)
		msleep(1000);
}
		
void InizializzaGPIOcanIndex(void)
{
	gpio_unexport(68);
	gpio_export(68);
	gpio_set_dir("/sys/class/gpio/gpio68/direction", "out");
	gpio_set_value("/sys/class/gpio/gpio68/value", "1");	
}

void LeggiStatoUsuraMMC()
{
	FILE *fp;
	char buff[256] {0};
	LifeTimeMMC_1to10 = 0;
	//Leggi stato usura MMC(life time) 0=nommc 1 to 10=mmc end life time
	fp = popen("mmc extcsd read /dev/mmcblk1 | grep LIFE_TIME_EST_TYP_A", "r");
	if (fgets(buff, sizeof(buff), fp) != NULL)
	{
		LifeTimeMMC_1to10 = GetValParFromString(buff, (char*)":", 2);
		pclose(fp);
		dmesgDebug("LeggiStatoUsuraMMC()", "");
		char str[12];
		snprintf(str, sizeof(str), "%d", LifeTimeMMC_1to10);
		dmesgDebug("StatoUsuraMMC(0=nommc tra 1 e 9 good , 10=usurata) n=", str);
		return;
	}
	pclose(fp);
}

void RuninDiskOrMMc()
{	
	FILE *fp;
	char buff[256] {0};
	LifeTimeMMC_1to10 = 0;
	fp = popen("findmnt / | grep /dev/mmcblk0p1", "r");
	if (fgets(buff, sizeof(buff), fp) != NULL)
		RunDisk = RUNSD;
	else
	{
		RunDisk = RUNMMC;
		LeggiStatoUsuraMMC();
	}
	pclose(fp);
}

int TimerRitVerLic=0;
const int TEMPORITVERLIC = 350; //20 secondi
void VerificaLicenze(void)
{
	FILE *fpd;
	char buffdemo[256] {0};
	char bufflice[256] {0};
	char bufftool[256] {0};
	char*  Demopresent = NULL;
	char*  Licepresent = NULL;
	char*  Toolpresent = NULL;
	char*  Errpresent = NULL;
	
	if (StopRuntime==true) return;
	if (TimerRitVerLic > TEMPORITVERLIC) return;
	if (TimerRitVerLic == TEMPORITVERLIC)  
	{		
		if (TipoPlc == PLCLOGICLAB)
		{	
			if (FileExist("/tmp/plc.status"))
			{
				fpd = popen("grep 'LICENSED' /tmp/plc.status", "r");
				Licepresent = fgets(bufflice, sizeof(bufflice), fpd);
				pclose(fpd);
				//se LICENSED present lic. OK 
				if (Licepresent != NULL)
				{
					//touch("/tmp/LogicLabok");
					dmesgDebug("LogicLab Lic. OK", "");
				}
				else
				{
					dmesgDebug("Logiclab Lic. !! KO !!", "");
					blampallled = true;
					LicPresent = false;
				}
			}
			else
			{
				dmesgDebug("Logiclab Lic. !! KO !!", "");
				blampallled = true;
				LicPresent = false;
			}
		}
		
		if (TipoPlc == PLCCODESYS)
		{			
			if (FileExist("/tmp/codesyscontrol.log") == true)
			{
				//file LOG presente
				//test se runtime usa SL license
				if (DirExist("/data/codesys/cmact_licenses") == true || DirExist("/data/codesys/.cmact_licenses") == true)
				{
					// SL license
					fpd = popen("grep 'runtime licensed' /tmp/codesyscontrol.log", "r");
					Licepresent = fgets(bufflice, sizeof(bufflice), fpd);
					fpd = popen("grep 'demo' /tmp/codesyscontrol.log", "r");
					Demopresent = fgets(buffdemo, sizeof(buffdemo), fpd);	
				}
				else
				{
					//Toolkit license
					fpd = popen("grep 'PL700-' /tmp/codesyscontrol.log", "r");
					Toolpresent = fgets(bufftool, sizeof(bufftool), fpd);
					fpd = popen("grep 'Demo' /tmp/codesyscontrol.log", "r");
					Demopresent = fgets(buffdemo, sizeof(buffdemo), fpd);	
				}
				fpd = popen("grep ' ERROR' /tmp/codesyscontrol.log", "r");
				Errpresent = fgets(bufflice, sizeof(bufflice), fpd);
				pclose(fpd);
			}
			else if (FileExist("/data/codesys/codesyscontrol.log") == true)
			{
				//file LOG presente
				//test se runtime usa SL license
				if (DirExist("/data/codesys/cmact_licenses") == true || DirExist("/data/codesys/.cmact_licenses") == true)
				{
					// SL license
					fpd = popen("grep 'runtime licensed' /data/codesys/codesyscontrol.log", "r");
					Licepresent = fgets(bufflice, sizeof(bufflice), fpd);
					fpd = popen("grep 'demo' /data/codesys/codesyscontrol.log", "r");
					Demopresent = fgets(buffdemo, sizeof(buffdemo), fpd);	
				}
				else
				{
					//Toolkit license
					fpd = popen("grep 'Pixsys ARM-Linux-SM' /data/codesys/codesyscontrol.log", "r");
					Toolpresent = fgets(bufftool, sizeof(bufftool), fpd);
					fpd = popen("grep 'Demo' /data/codesys/codesyscontrol.log", "r");
					Demopresent = fgets(buffdemo, sizeof(buffdemo), fpd);	
				}
				fpd = popen("grep ' ERROR' /data/codesys/codesyscontrol.log", "r");
				Errpresent = fgets(bufflice, sizeof(bufflice), fpd);
				pclose(fpd);
			}
			else 
			{
				dmesgDebug("codesyscontrol.log not exist", "");
				return;
			}
			
			//messaggi e led di info
			if (Licepresent != NULL || Toolpresent != NULL)
			{
				touch("/tmp/codesyslicok");
				if (Toolpresent != NULL)
					dmesgDebug("Codesys TOOLKIT Pixsys ARM-Linux-SM Lic. OK", "");
				else
					dmesgDebug("Codesys SL Lic. OK", "");
				if (Demopresent != NULL )
				{
					dmesgDebug("Codesys Exp Lic.  !! KO !!", "");
				}
				if (Demopresent != NULL || Errpresent != NULL)
				{
					LedTRMinf |=  LED_W_1; //
					LedBLKinf |=  LED_W_1; //
				}
			}
			else
			{
				dmesgDebug("Codesys Lic. !! KO !!", "");
				blampallled = true;
				LicPresent = false;
			}
		}
		
		if (TipoPlc == CNV600)
		{
			dmesgDebug("CNV600 Lic. OK", "");
		}			
		dmesgDebug("", "");
		TimerRitVerLic++; //stop timer
	}
	else
	{
		TimerRitVerLic++;	
	}
}

int ritreboot = 0;


void GestioneReboot()
{
	if (FileExist("/tmp/reboot") && ritreboot==0)
	{
		ritreboot++;
		//msleep(6000); //fa il reboot solo se buco rete
		//system("reboot");
		//reboot(2);
		syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART, NULL);
		CiclaSuWhile = false;
		/* Enter the critical section -- other threads are locked out */
//		pthread_mutex_lock(&cs_mutex);
//		
		
//		//print stop time Terminal
//		FILE *pFile;
//		time_t t = time(NULL);
//		struct tm tm = *localtime(&t);
//		pFile = fopen("/data/ups.txt", "a");
//		fprintf(pFile, "rebo: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
//		fclose(pFile);
//		//
//		//sync();
//		if (PIDisRun(pidplc) == true) kill(pidplc, SIGKILL);
//		/*Leave the critical section -- other threads can now pthread_mutex_lock()  */
//		pthread_mutex_lock(&cs_mutex);
//		kill(pidplc, SIGKILL);
//		sync();
//		pthread_mutex_unlock(&cs_mutex);
	}
}	

void Gestione24VOFF()
{
	//T7=67 (VMON) UPS per SHUTDOWN 1= ON
	int T7  = gpio_get_value("/sys/class/gpio/gpio67/value");	
	if (T7 == 0)
	{
		if(ritreboot++ == 10)
		{
			//print stop time Terminal
			FILE *pFile;
			time_t t = time(NULL);
			struct tm tm = *localtime(&t);
			pFile = fopen("/data/ups.txt", "a");
			fprintf(pFile, "rebo: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			fclose(pFile);
			//sync();
			system("reboot");
		}
		//system("echo '+24V_OFF ' $(date) >> /data/poweroff.txt");
		//sync();
		//if (ritreboot >= 2) //500ms //2 per prova
		//{
		//	PlPowerOff = 1;
		//	system("reboot");
		//}
#ifdef UPS
		system("echo '+24V_OFF_I ' $(date) >> /data/mounttime.txt");
		/* Enter the critical section -- other threads are locked out */
		pthread_mutex_lock(&cs_mutex);
		sync();
		if (PIDisRun(pidplc) == true)
			kill(pidplc, SIGKILL);
		/*Leave the critical section -- other threads can now pthread_mutex_lock()  */
		pthread_mutex_unlock(&cs_mutex);
		system("echo '+24V_OFF_F ' $(date) >> /data/mounttime.txt");
		system("reboot");
#endif 
	}
	else
		ritreboot=0;
}	


void LampeggioAllOn(int s)
{	
	(void)s;
	blampallled = true;
}

void LampeggioAllOff(int s)
{	
	(void)s;
	blampallled = false;
}

#define MAX_IP_LENGTH 64
#define FILE_NAME "/data/pmb100/ips_llexeckey.txt"
#define TEST_PORT 4999   // Porta su cui testare la connessione
#define TIMEOUT_SEC 2    // Timeout in secondi

// Funzione per verificare se un host è raggiungibile su una determinata porta con timeout
int is_host_reachable(const char *ip, int port) {
	int sock;
	struct sockaddr_in server;
	struct timeval timeout;
	fd_set fdset;

	// Creazione socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Errore nella creazione della socket");
		return 0;
	}

	// Imposta la socket come non bloccante
	fcntl(sock, F_SETFL, O_NONBLOCK);

	// Configura la struttura dell'indirizzo del server
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	if (inet_pton(AF_INET, ip, &server.sin_addr) <= 0) {
		perror("Indirizzo IP non valido");
		close(sock);
		return 0;
	}

	// Tentativo di connessione
	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		if (errno != EINPROGRESS) {
			close(sock);
			return 0; // Connessione fallita immediatamente
		}
	}

	// Imposta il timeout
	FD_ZERO(&fdset);
	FD_SET(sock, &fdset);
	timeout.tv_sec = TIMEOUT_SEC;
	timeout.tv_usec = 0;

	// Controlla se la connessione è riuscita entro il timeout
	if (select(sock + 1, NULL, &fdset, NULL, &timeout) > 0) {
		int so_error;
		socklen_t len = sizeof(so_error);
		getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
		if (so_error == 0) {
			close(sock);
			return 1; // Connessione riuscita
		}
	}

	close(sock);
	return 0; // Timeout o connessione fallita
}

int LicenziaLogicLabSuServerAttraversoPCAppoggio() {
	FILE *file = fopen(FILE_NAME, "r");
	if (!file) {
		printf("Errore nell'apertura del file");
		return 1;
	}
	char ip[MAX_IP_LENGTH];
	printf("Verifica della raggiungibilità degli IP...\n");
	while (fgets(ip, sizeof(ip), file)) {
		// Rimuove eventuali caratteri di nuova riga
		ip[strcspn(ip, "\r\n")] = 0;
		if (strlen(ip) == 0) {
			continue; // Salta righe vuote
		}
		if (is_host_reachable(ip, TEST_PORT)) {
			printf("Host %s è raggiungibile sulla porta %d\n", ip, TEST_PORT);
			char cmd[255];
			sprintf(cmd ,"/data/plc/AxelRTLicenseClient %s", ip);
			system(cmd);
			return 0;
		}
		else {
			printf("Host %s NON è raggiungibile sulla porta %d\n", ip, TEST_PORT);
		}
	}
	fclose(file);
	return 1;
}

bool StartFinder = false;
bool RunCodesysPerCollaudo;
bool StopCodesysPerCollaudo;
bool RunBarCodePLPerFinalized;
int ritstartplc = 0;
bool FinalizedAndLicensed = false;
bool finalizedok = false;

void GestioneDeviceConTerminale()
{	
	if (PlPowerOff == 0)	//gestione terminale al POWER_ON
	{			
		if (TerminaleOnlineAtStart == true) 
		{
			TimerStopLampeggioAllled(LicPresent);
			TimerStopLampeggioAllledInf(LicPresent);
			//******************************************** plc Run SD ************************
			if (Modalita == MODALITA_RUNSD)
			{	
				system("mkdir /media/emmc");
				system("mount /dev/mmcblk1p1 /media/emmc");
				system("cp /media/emmc/etc/pixsys/device.info /etc/pixsys/device.info");
				system("cp /media/emmc/data/plc/LLExec.key /data/plc/LLExec.key");
				LeggiFileInfo();
				if (TipoPlc == PLCCODESYS)
				{
					PreparaEmmc_Sd_PerCODESYS();
				}
				if (TipoPlc == PLCLOGICLAB)
				{
					system("rm -r /data/codesys/*");
					system("rmdir /data/codesys");
					system("rm -r /data/edge/*");
					system("rmdir /data/edge");
					remove("/etc/systemd/system/codesysedge.service");
				}
				remove("/data/runsd");
				//Modalita = MODALITA_RUN;
				touch("/tmp/reboot");
				sync();
			}
			//******************************************** plc in funzionamento normale ************************
			if (Modalita == MODALITA_RUN)
			{	
				GestionePulsanteS1();
				GestioneDipIp99();
				GestioneDipVPN();
				GestioneDipNFC();
				GestioneledDISK();
				GestioneledNET();
				InizializzaledSERIALI();
				if (TipoPlc == PLCCODESYS || TipoPlc == PLCLOGICLAB) 
				{	
					GestionePLC();
					if (ritstartplc >= 200) //20sec ritardo lunch test licenza PLC runtime
					{	
						VerificaLicenze();	
					}
					else ritstartplc++;
				}
			}
			//******************************************** Upgrade da SD
			if (Modalita == MODALITA_UPGRADE)
			{
				if (RunDisk == RUNSD)
				{
					//esci se copia è stata lanciata
					if (copy2emmc == false)
					{
						copy2emmc = true;
						//cancella eventuali file da non copiare
						remove("/data/collaudook");
						remove("/data/finalized");
						remove("/data/poweroff.txt");
						remove("/data/codesyscontrol.log");
						remove("/tmp/sh.ok");
						LedTRMinf |= LED_INTERNO  | LED_E_1 | LED_E_2 | LED_W_1 | LED_DISCO; //
						LedBLKinf |=  LED_INTERNO | LED_E_1 | LED_E_2 | LED_W_1 | LED_DISCO; //
						//se EMMC non formattata SLC: fornatta e crea file "/data/emmcslc.ok" che serve per partenza automatica al riavvio dopo power off
						//se EMMC è gia formattata procede subito con la copia 
						//In fine copia se tutto ok , genera file sh.ok
						system("/opt/backup/uboot/copy2emmc.sh ext4 persistent");
						sync();
						if (FileExist("/tmp/sh.ok") == true)
						{	
							LedBLKinf &= ~(LED_INTERNO | LED_E_1 | LED_E_2 | LED_W_1 | LED_DISCO); //se ok stop lampeggio
						}
						else
						{
							//lampeggio all per ERRORE
							blampallled = true;
							LedTRMinf &= ~(LED_INTERNO | LED_E_1 | LED_E_2 | LED_W_1 | LED_DISCO); //se ok stop led
							LedBLKinf &= ~(LED_INTERNO | LED_E_1 | LED_E_2 | LED_W_1 | LED_DISCO); //se ok stop lampeggio
						}
					}			
					sync();
				}
				if (RunDisk == RUNMMC)
				{
					if (TipoPlc == PLCCODESYS)
					{
						PreparaEmmc_Sd_PerCODESYS();
						system("systemctl stop codesysedge.service");
						system("systemctl disable codesysedge.service");
					}
					if (TipoPlc == PLCLOGICLAB)
					{	
						system("rm -r /data/codesys/*");
						system("rmdir /data/codesys");
						system("rm -r /data/edge/*");
						system("rmdir /data/edge");
					}
					remove("/etc/systemd/system/codesysedge.service");
					remove("/data/upgrade");
					remove("/data/disablecanerror");
					Modalita = MODALITA_RUN;
					sync();
				}
			}
			//************************************************ plc in collaudo *********************************
			if (Modalita == MODALITA_COLLAUDO)
			{	
				GestioneDipNFC();
				GestioneledDISK();
				GestioneledNET();
				InizializzaledSERIALI();
				if (RunDisk == RUNSD)
				{	
					//Se è una SD di collaudo si deve chiamare plsd
					char hostname[32];
					gethostname(hostname, 32);
					if ((strcmp(hostname, "plsd") != 0) && (TipoPlc == NOCONF))
					{
						//nuovo hostname
						system("hostnamectl set-hostname 'plsd'");
						system("hostnamectl set-hostname 'plsd' --pretty");
						system("cp /data/pmb100/devicefinder.wait /etc/systemd/system/devicefinder.service");
						system("cp /data/pmb100/device.plsd /etc/pixsys/device.info");
						system("cp /data/pmb100/.features.env_pl /data/PixsysPortal/.features.env");
						system("cp /data/pmb100/.preferences.env_pl /data/PixsysPortal/.preferences.env");
						//remove eth0.1
						if (FileExist("/etc/NetworkManager/system-connections/eth0.1.nmconnection") == true)
						{	
							system("nmcli connection down eth0.1");
							remove("/etc/NetworkManager/system-connections/eth0.1.nmconnection");
						}	
						//remove eth0.2
						if (FileExist("/etc/NetworkManager/system-connections/eth0.2.nmconnection") == true)
						{	
							system("nmcli connection down eth0.2");
							remove("/etc/NetworkManager/system-connections/eth0.2.nmconnection");
						}							
						//eth0
						if (FileExist("/etc/NetworkManager/system-connections/eth0.nmconnection") == false)
						{	
							system("nmcli con add type ethernet ifname eth0 con-name eth0 ipv4.method auto");
							system("nmcli connection up eth0");
						}
						sync();
						sleep(2);
						//reboot
						touch("/tmp/reboot");
						return;
					}
					if (TipoBoard == BOARD340)
					{
						GestionePulsanteS1_collaudo_RUNSD();
						//stop di questo servizio per liberare seriale per visualizzare msg DEBUG del KERNEL
						if (Dip == 0x11) //chiudi Main con Dip stop e pulsante S1 premuto se RUN in SD
						{	
							system("systemctl stop pmb100-terminal.service"); //stop servizio LED
							StartDebug_tty0();
							CiclaSuWhile = false;
							return ; //esci
						}
						//start codesys per prima Verifica hardware prima di copia in EMMC
						if (Dip == 0x0 && RunCodesysPerCollaudo == false)
						{
							dmesgDebug("Start CODESYS collaudo", "");
							system("systemctl start codesyscontrol.service");
							RunCodesysPerCollaudo = true;
							blampallledinf = true;
						}
						//ferma il codesys per copia su EMMC
						if (Dip != 0x0 && RunCodesysPerCollaudo == true)
						{
							dmesgDebug("Stop CODESYS collaudo", "");
							system("systemctl stop codesyscontrol.service");
							remove("/data/collaudook");
							RunCodesysPerCollaudo = false;
							blampallledinf = false;
						}
					}	
				}
				if (RunDisk == RUNMMC)
				{	
					GestionePulsanteS1_collaudo_RUNMMC();
					if (FileExist("/data/finalized") == true)
					{
						if(finalizedok==false)
						{	
							LeggiFileInfo();
							finalizedok = true;
						}
						//licenzia logiclab
						if(TipoPlc == PLCLOGICLAB)
						{	
							if (FileExist("/data/plc/LLExec.key") == false)
							{
								LicenziaLogicLabSuServerAttraversoPCAppoggio();
								sleep(2);
								blampallled = true;
							}
							else
							{
								FinalizedAndLicensed = true;
								blampallled = false;
							}
						}
						//licenzia codesys
						if(TipoPlc == PLCCODESYS){	FinalizedAndLicensed = true; }
						//Test hardware
						if (Dip == 0x0B) //1011
						{
							if (StopCodesysPerCollaudo == false)
							{
								dmesgDebug("Stop CODESYS collaudo", "");
								system("systemctl stop codesyscontrol.service");
								StopCodesysPerCollaudo = true;
							}
						}
						if (Dip == 0x04 && FinalizedAndLicensed == true) //0100
						{
							if (RunCodesysPerCollaudo == false)
							{
								dmesgDebug("Start CODESYS collaudo", "");
								system("systemctl start codesyscontrol.service");
								RunCodesysPerCollaudo = true;

								//print start time PLC
								FILE* pFile;
								time_t t = time(NULL);
								struct tm tm = *localtime(&t);
								pFile = fopen("/data/ups.txt", "a");
								fprintf(pFile, "RunPL: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
								fclose(pFile);
							}
						}
					}
					else
					{
						//tutti i dip a 1 per collaudo senza finalizzazione
						if (Dip == 0x0F && RunCodesysPerCollaudo == false) //1111
						{
							dmesgDebug("Start CODESYS collaudo", "");
							system("systemctl start codesyscontrol.service");
							RunCodesysPerCollaudo = true;
						}
						//lanci programma per finalizzare con lettore QRcode su USB
						if (Dip == 0x04 && RunBarCodePLPerFinalized == false) //0100
						{
							dmesgDebug("Stop CODESYS collaudo", "");
							system("systemctl stop codesyscontrol.service");
							//
							dmesgDebug("Start BarCodePL for finalized", "");
							system("/data/pmb100/BarCodePL &");
							RunBarCodePLPerFinalized = true;
						}
					}
				}
			}
		}
		//gestione mancanza terminale LED ma è previsto
		else 
		{
			if (Modalita == MODALITA_RUN || Modalita == MODALITA_UPGRADE)
			{
				if (TipoBoard == BOARD340)
				{
					//gestione start PLC + VPN se all'avvio manca il terminale 
					RitardoStartPLCSeNonPresenteTRM++;
					if (RitardoStartPLCSeNonPresenteTRM >= 10)
					{
						//disabilita comunicazione
						InizializzaTerminaleLedUart(false);
						RitardoStartPLCSeNonPresenteTRM = 10;
						Dip = BIT_DIP4_VPN; //start PLC
						if (TipoPlc == PLCCODESYS || TipoPlc == PLCLOGICLAB) GestionePLC();
					}
				}
			}
		}
	}			
}


void GestioneDeviceSenzaTerminale(void)
{	
	if (Modalita == MODALITA_COLLAUDO)
	{
		if (RunDisk == RUNSD)
		{
			//Se è una SD di collaudo si deve chiamare pphsd
			char hostname[32];
			gethostname(hostname, 32);
			if ((strcmp(hostname, "pphsd") != 0) && (TipoPlc == NOCONF))
			{	//nuovo hostname
				system("hostnamectl set-hostname 'pphsd'");
				system("hostnamectl set-hostname 'pphsd' --pretty");
				system("cp /data/pmb100/devicefinder.nowait /etc/systemd/system/devicefinder.service");
				system("cp /data/pmb100/device.pphsd /etc/pixsys/device.info");
				system("cp /data/pmb100/.features.env_pph /data/PixsysPortal/.features.env");
				system("cp /data/pmb100/.preferences.env_pph /data/PixsysPortal/.preferences.env");
				
//				//remove eth0
//				system("nmcli connection down eth0");
//				remove("/etc/NetworkManager/system-connections/eth0.nmconnection");
//				//Vlan1
//				system("nmcli connection add type vlan con-name eth0.1 ifname eth0.1 dev eth0 id 1");
//				system("nmcli connection modify eth0.1 ipv4.method auto");
//				//Vlan2
//				system("nmcli connection add type vlan con-name eth0.2 ifname eth0.2 dev eth0 id 2");
//				system("nmcli connection modify eth0.2 ipv4.addresses 192.168.10.1/24");
//				system("nmcli connection modify eth0.2 ipv4.method manual");
//				system("nmcli connection up eth0.1");
//				system("nmcli connection up eth0.2");
				
				sync();
				//reboot
				touch("/tmp/reboot");
				return;
			}
			
			//esci se copia è stata lanciata
			if (copy2emmc == false)
			{
				copy2emmc = true;
				//cancella eventuali file da non copiare
				remove("/data/collaudook");
				remove("/data/finalized");
				remove("/data/poweroff.txt");
				remove("/data/codesyscontrol.log");
				remove("/tmp/sh.ok");
			
				//se EMMC non formattata SLC: fornatta e crea file "/data/emmcslc.ok" che serve per partenza automatica al riavvio dopo power off
				//se EMMC è gia formattata procede subito con la copia 
				//In fine copia se tutto ok , genera file sh.ok
				system("/opt/backup/uboot/copy2emmc.sh ext4 nopersistent");
				sync();
				if (FileExist("/tmp/sh.ok") == true)
				{	
					system("systemctl stop devicefinder.service"); //stop servizio finder
				}
				else
				{
					//lampeggio all per ERRORE
				}
			}					
		}	
		if (RunDisk == RUNMMC)
		{
			//connetti su eth0
			if (FileExist("/etc/NetworkManager/system-connections/eth0.nmconnection") == false)
			{
//				//creo il file eth0.connection se non presente
				system("nmcli connection add con-name eth0 ifname eth0 type ethernet");
//				//applico new IP
				system("nmcli connection mod eth0 ipv4.method auto");
				system("nmcli connection up eth0");
				system("nmcli device reapply eth0"); //applica subito le modifiche
			}
			//lanci programma per finalizzare con lettore QRcode su USB
			if (RunBarCodePLPerFinalized == false)
			{
				dmesgDebug("Start BarCodePL for finalized", "");
				system("/data/pmb100/BarCodePL &");
				RunBarCodePLPerFinalized = true;
			}
			//se battezzato riparti
			if (FileExist("/data/finalized") == true)
			{
				LeggiFileInfo(); //Aggiorna Nome
				if( TipoPlc == PPH600)
				{
					//enable PixsysPortal al riavvio
					touch("/data/finalized_end");
					remove("/data/finalized");
					remove("/data/collaudo");
					//delete file collaudo che non servono
					//togliere /* e togliere rmdir non serve piu
					system("rm -rf /data/plc");
					system("rm -rf /data/codesys");
					//system("rmdir /data/plc"); 
					//system("rmdir /data/codesys");
					//
					//remove eth0
					system("nmcli connection down eth0");
					//system("nmcli connection delete eth0");
					system("nmcli connection modify eth0 connection.autoconnect no");
					system("nmcli connection modify eth0 connection.autoconnect-slaves 0");
					//Vlan1
					system("nmcli connection add type vlan con-name eth0.1 ifname eth0.1 dev eth0 id 1");
					system("nmcli connection modify eth0.1 ipv4.method auto");
					//Vlan2
					system("nmcli connection add type vlan con-name eth0.2 ifname eth0.2 dev eth0 id 2");
					system("nmcli connection modify eth0.2 ipv4.addresses 192.168.10.1/24");
					system("nmcli connection modify eth0.2 ipv4.method manual");
					system("nmcli connection up eth0.1");
					system("nmcli connection up eth0.2");
					//	
					sync();
					//reboot
					touch("/tmp/reboot");
					return;
				}
			}	
		}
	}
	if (Modalita == MODALITA_UPGRADE)
	{
		if (RunDisk == RUNSD)
		{		
			//esci se copia è stata lanciata
			if (copy2emmc == false)
			{
				copy2emmc = true;
				//cancella eventuali file da non copiare
				remove("/data/collaudook");
				remove("/data/finalized");
				remove("/data/poweroff.txt");
				remove("/data/codesyscontrol.log");
				remove("/tmp/sh.ok");	
				//se EMMC non formattata SLC: fornatta e crea file "/data/emmcslc.ok" che serve per partenza automatica al riavvio dopo power off
				//se EMMC è gia formattata procede subito con la copia 
				//In fine copia se tutto ok , genera file sh.ok
				system("/opt/backup/uboot/copy2emmc.sh ext4 persistent");
				sync();
				if (FileExist("/tmp/sh.ok") == true)
				{	
					system("systemctl stop devicefinder.service"); //stop servizio finder
				}
				else
				{
					// ERRORE
				}
			}				
		}
		if (RunDisk == RUNMMC)
		{
			if (TipoPlc == PPH600)
			{	
				if (FileExist("/data/finalized_end") == false)
				{
					//enable PixsysPortal al riavvio
					touch("/data/finalized_end");
					remove("/data/upgrade");
					remove("/data/disablecanerror");
					//delete file collaudo che non servono
					system("rm -rf /data/plc");
					system("rm -rf /data/codesys");
					touch("/data/upgradeok");
					sync();
					//reboot
					touch("/tmp/reboot");
					return;
				}
			}	
		}	
	}
	if (Modalita == MODALITA_RUN)
	{	
		if (RunDisk == RUNMMC)
		{
			if (TipoPlc == PPH600)
			{
				if (FileExist("/data/finalized_end") == true)
				{
					//enable PixsysPortal
					system("systemctl enable pixsysportal.service");
					system("systemctl start pixsysportal.service");
					//enable servizio gateway codesys
					system("cp /data/edge/codesysedge /etc/init.d/codesysedge");
					system("systemctl enable codesysedge.service");
					system("systemctl start codesysedge.service");
					remove("/data/finalized_end"); //disabilito questo IF
					if (FileExist("/data/upgradeok") == true)
					{
						touch("/tmp/registrazioneok");
					}
					//system("systemctl stop pmb100-terminal.service");    //stop servizio 
					sync();
					//reboot
					//touch("/tmp/reboot");
					//return;
				}
				if (FileExist("/tmp/registrazioneok") == true )
				{
					remove("/data/PixsysPortal/registra.sh");
					remove("/tmp/registrazioneok");
					//terminal.bin non serve più posso fermarlo
					system("systemctl disable pmb100-terminal.service"); //disable servizio
					sync();
				}	
				else
				{
					//registra device per collaudo
					if (FileExist("/data/PixsysPortal/registra.sh") == true)
					{
						system("/data/PixsysPortal/registra.sh");
					}
					sleep(5);
				}
			}	
		}
	}
}


int main(int argc, char *argv[])
{
	(void)argc;
	//char strVer[] = "Ver. 1.8 *** 11/06/2025\n";
	char strVer[] = "Ver. 1.9 *** 12/09/2025\n";

	//setpriority(PRIO_PROCESS, 0, -2);
	//recupera nome .bin con path
	BinName = basename(argv[0]);
	dmesgDebug("*** START ***", strVer);
	
	//print start time Terminal
	FILE *pFile;
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	pFile = fopen("/tmp/StartTimeTerminal.log", "a");
	fprintf(pFile, "start: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	fclose(pFile);
	//pFile = fopen("/data/StartTimeTerminal.log", "a");
	//fprintf(pFile, "start: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	//fclose(pFile);

	//*******Terminal.bin***************************************************************//
	signal(SIGUSR1, LampeggioAllOn);
	signal(SIGUSR2, LampeggioAllOff);
	InizializzaGPIO();
	LeggiFileInfo();
	RuninDiskOrMMc();
	if (TipoBoard == BOARD335)InizializzaTerminaleLedSpi();  //task Spi
	if (TipoBoard == BOARD340)InizializzaTerminaleLedUart(true); //task ttyS0
	
	//flag PLC RUN SD
	if (FileExist("/data/runsd") == true  && RunDisk == RUNSD) Modalita = MODALITA_RUNSD;
	//flag PLC in collaudo
	if (FileExist("/data/collaudo") == true) Modalita = MODALITA_COLLAUDO;
	//flag PLC in upgrade
	if (FileExist("/data/upgrade") == true) Modalita = MODALITA_UPGRADE;
	//flag PLC in autorun one shot
	if (FileExist("/data/oneshot.sh") == true) { system("/data/oneshot.sh"); remove("/data/oneshot.sh"); }
	//flag PLC in autorun 
	if (FileExist("/data/autorun.sh") == true) system("/data/autorun.sh");
	
	//GestioneNumerazioneNodiCan
	if (TipoPlc == PLCCODESYS || TipoPlc == PLCLOGICLAB || Modalita == MODALITA_COLLAUDO)
	{
		if (TipoBoard == BOARD340) 
			if(FileExist("/tmp/PLE500present")==false)
				GestioneNumerazioneNodiCan();
	}

	//******Loop Main Terminal.bin*/
	dmesgDebug("Loop Main sleep 100ms","");
	while (CiclaSuWhile)
	{
		if (TipoBoard == BOARD340 || TipoBoard == BOARD335) GestioneDeviceConTerminale();
		if (TipoBoard == BOARDPPH) GestioneDeviceSenzaTerminale();
		//Gestione24VOFF();				//reboot se UPS is ON
		GestioneReboot(); //se trova /tmp/reboot  riparte
		msleep(100);						//loop main a 100ms
	}//while 1
	return 0;
}//main
