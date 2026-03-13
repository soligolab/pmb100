#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>

#include "terminale_led.h"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

char buffer[EVENT_BUF_LEN];
int fdser;
bool InitLedSer;
pthread_t thledser;


void GestioneledSER(void)
{	
	int length, i = 0;
	if (InitLedSer == true)
	{
		// bloccante ???
		length = read(fdser, buffer, EVENT_BUF_LEN);
		if (length <= 0) return;
		/*actually read return the list of change events happens. Here, read the change event one by one and process it accordingly.*/
		while (i < length)
		{
			struct inotify_event *event = (struct inotify_event *) &buffer[i]; 
			if ((event->mask == IN_OPEN)  || (event->mask == IN_CREATE) )
			{
				if (event->wd == 1) LedTRMsup |= LED_COM1; //on led com1
				if (event->wd == 2) LedTRMsup |= LED_COM2; //on led com2
			}
			else if ((event->mask == IN_CLOSE) || (event->mask == IN_CLOSE_NOWRITE) || (event->mask == IN_CLOSE_WRITE))
			{
				if (event->wd == 1) LedTRMsup &= ~LED_COM1;  //off led com1
				if (event->wd == 2) LedTRMsup &= ~LED_COM2;  //off led com2
				
			}
			i += EVENT_SIZE + event->len;
		}	
	}		
}	

void *th_ledser(void * arg) //task 10ms
{
	while (true)
	{
		GestioneledSER();
	}
	pthread_exit(0);
}

void InizializzaledSERIALI(void)
{
	int wd;
	if (InitLedSer == false)
	{
		/*creating the INOTIFY instance*/
		
		
		//Accendi LED se porte già in uso
		fdser = inotify_init();
		wd = inotify_add_watch(fdser, "/dev/ttyS2", IN_OPEN | IN_CLOSE | IN_CLOSE_NOWRITE | IN_CLOSE_WRITE | IN_CREATE);
		wd = inotify_add_watch(fdser, "/dev/ttyS4", IN_OPEN | IN_CLOSE | IN_CLOSE_NOWRITE | IN_CLOSE_WRITE | IN_CREATE);
		int status = pthread_create(&thledser, NULL, th_ledser, NULL);
		InitLedSer = true;
	}
}