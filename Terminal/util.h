#pragma once

#ifndef _UTIL_H
#define _UTIL_H

int GetValParFromString(char * buffstring, char * sepa, int numeropar);
char* GetStrParFromString(char * buffstring, char * sepa, int numeropar);
char * GetGrupParSubParFromFile(const char * filename,const char * gruppar,const char * subpar ,char retvalue[]);
void dmesgDebug(const char *msg1, const char *msg2);
bool FileExist(const char * fname);
bool DirExist(const char * Dirname);
bool touch(const char *filename);
bool touchstr(const char *filename, const char *str);
pid_t GetPid(char * filename);
bool PIDisRun(pid_t pid);
bool CercaStrSuFile(char * filename, char * findkey, char * str);



//#define DECSTR 0
//#define HEXSTR 1

#endif // _UTIL_H