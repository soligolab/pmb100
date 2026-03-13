#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>
#include <errno.h> 

//#include "Ptype.h"

#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

#include <signal.h>
#include <sys/stat.h>

#include <fcntl.h> // open function
#include "sys/syscall.h"


extern char * BinName;

void dmesgDebug(const char *msg1, const char *msg2)
{
	int fd_kmsg = open("/dev/kmsg", O_WRONLY);
	FILE * f_kmsg = fdopen(fd_kmsg, "w");
	fprintf(f_kmsg, "<%u>%s: %s %s", 7,BinName, msg1,msg2);
	fflush(f_kmsg);
	fclose(f_kmsg); 
}

/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec)
{
	struct timespec ts;
	int res;
	if (msec < 0)
	{
		errno = EINVAL;
		return -1;
	}
	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;
	do {
		res = nanosleep(&ts, &ts);
	} while (res && errno == EINTR);
	return res;
}


bool PIDisRun(pid_t pid)
{	
	struct stat sts;
	if (pid == 0) return false;
	char  str[16];
	sprintf(str, "/proc/%d", pid);
	if (stat(str, &sts) == -1 && errno == ENOENT)
	{
		// process doesn't exit
		return false;
	}
	return true; 
}

pid_t GetPID(char * filename)
{
	FILE *cmd_pipe;
	char buf[512];
	char strpopen[64];
	sprintf(strpopen, "pidof %s", filename);
	cmd_pipe = popen(strpopen, "r");
	fgets(buf, 512, cmd_pipe);
	pid_t pid = strtoul(buf, NULL, 10);
	pclose(cmd_pipe);
	return pid;
}


bool isSpace(char c)
{
	switch (c) 
	{
		case ' ':
		case '\n':
		case '\t':
		case '\f':
		case '\r':
			return true;
			break;
		default:
			return false;
			break;
	}
}

char* trim(char* input)
{
	char* start = input;
	while (isSpace(*start)) 
	{
		//trim left
		start++;
	}
	char* ptr = start;
	char* end = start;
	while (*ptr++ != '\0')
	{
		//trim right
		if (!isSpace(*ptr)) 
		{
			//only move end pointer if char isn't a space
			end = ptr;
		}
	}
	*end = '\0'; //terminate the trimmed string with a null
	return start;
}


char* GetStrParFromString(char * buffstring, char * sepa, int numeropar)
{
	char *token;
	int n = 0;
	token = strtok(buffstring, sepa);
	//cerca il parametro
	while (token != NULL)
	{
		n++;
		if (n == numeropar)	break;
		token = strtok(NULL, sepa);
	}
	return token;	
}

int GetValParFromString(char * buffstring, char * sepa, int numeropar )
{
	char *token;
	int n = 0;
	token = strtok(buffstring,sepa);
	//cerca il parametro
	while (token != NULL)
	{
		n++;
		if (n == numeropar)	break;
		token = strtok(NULL, sepa);
	}
	unsigned int out;
	sscanf(token, "%x", &out);
	return out;	
}


int GetGrupParSubParFromFile(const char * filename,const char * gruppar,const char * subpar , char retvalue[])
{
	char *token;
	char* r;
	bool nogroup=true;
	static char buffer[1024] {0};
	int fd = open(filename, O_RDONLY);
	read(fd, &buffer, sizeof(buffer));
	close(fd);
	char* slf = (char*)"\n";
	char* sug = (char*)"=";
	token = strtok(buffer,slf);
	//cerca il Gruppo di parametri
	while (token != NULL)
	{	
		r = strcasestr(token, gruppar);
		if (r != NULL) break;
		token = strtok(NULL, slf);
	}
	if (token == NULL) return 0; //nessun gruppo trovato
	
	//cerca il parametro
	token = strtok(NULL, slf);  
	while (token != NULL)
	{
		if (strstr(token, "[") != NULL) return 0; //nessun parametro trovato
		r = strcasestr(token,subpar);
		if (r != NULL) break;
		token = strtok(NULL, slf);
	}
	if (token == NULL) return 0; //nessun parametro trovato
	
	// estrai valore da parametro
	strcpy(buffer,token);
	token = strtok(buffer, sug);
	token = strtok(NULL, sug);
	//togli tutti i caratteri di formatazione
	token=trim(token);
	strcpy(retvalue, token);
	return 1;
}

bool touch(const char *filename) {
	int fd = open(filename, O_CREAT | S_IRUSR | S_IWUSR);
	if (fd == -1) {
		return false;
	}
	close(fd);
	return true;
}

bool touchstr(const char *filename, const char *str) {
	remove(filename);
	int fd = open(filename, O_CREAT | O_RDWR);
	if (fd == -1) {
		return false;
	}
	int le = strlen(str);
	write(fd,str,le);
	close(fd);
	return true;
}


bool FileExist(const char * fname)
{
	if (access(fname, F_OK) == 0) {
		return true; // file exists
	}
	else {
		return false; // file doesn't exist
	}
}

bool DirExist(const char * Dirname)
{	
	DIR* dir = opendir(Dirname);
	if (dir) 
	{
		closedir(dir);
		return true; /* Directory exists. */
	}
	else if (ENOENT == errno)
	{
		return false; /* Directory does not exist. */
	}
	else
	{
		return false; /* opendir() failed for some other reason. */
	}
}

bool CercaStrSuFile(char * filename,char * findkey ,char * str)
{
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	fp = fopen(filename, "r");
	if (fp == NULL) return false;

	while ((read = getline(&line, &len, fp)) != -1) 
	{
		char * pl = strstr(line, findkey);
		if (pl != NULL)
		{
			strcpy(str,pl);
			return true;
		}
	}
	fclose(fp);
	return false;
}