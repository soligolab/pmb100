
#include "stdint.h"
#include "candef.h"
#include "def.h"
#include "ptype.h"
#include "var.h"


#define	TIMER_STOP							0xFFF0
#define	TEMPO_SCADUTO						0xFFF1
#define	CONTEGGIO_IN_SEC					CLCSEC


/*	Questa funzione che serve a creare la base tempi per i timer, va chiamata da un interrupt che deve aver periodo
	minore di 100 mS. In ogni caso, si veda il file "timer.h" per ulteriori spiegazioni sulla costante UN_DECIMO.
*/	 
void GestioneTimer(void) 
{
	Time++;
	CSec--;
	if (CSec == 0)
	{
		CSec = 10 * UN_DECIMO;
		ToSec++;
	}
}

/*	attivo un timer alla rovescia;
	nto		= numero del timer da attivare (bit 7 = 1 per timer in secondi,
			  0 per timer in centesimi di sec.);
	wait_t	= tempo da attendere (dipende dall'unita` di misura prescelta
			  col bit 7 di nto);
*/
void TimerOn(unsigned short nto, unsigned short wait_t)
{
	unsigned short to_s_d;
	to_s_d  =  nto & CONTEGGIO_IN_SEC  ?  ToSec : Time;
	nto	&= ~CONTEGGIO_IN_SEC;
	MaxTime[nto] = wait_t;
	CntTime[nto] = to_s_d;
}	

/* forzo in TIME_OUT un timer */
void ClearTimer(unsigned short nto)
{
	TimerOn(nto, 0);
	nto	&= ~CONTEGGIO_IN_SEC;
	MaxTime[nto] = TEMPO_SCADUTO;
}

/*	Funzione che effettua l'inizializzazione di tutti i timer software dichiarati da eseguire prima di 
	iniziare ad usare le altre funzioni di gestione dei timer
*/	
void InizializzaTimer(void)
{
	short i;
	for (i = 0; i < NUMERO_TIMER; i++)
	{
		ClearTimer(i);
	}
	CSec = 1;
	Time = 0x0000;
}

/*	disattivo un timer */
void TimerStop(unsigned short nto)
{
	TimerOn(nto, TIMER_STOP);
}


/*	verifico se un timer e` scaduto; ritorno != 0 in caso affermativo;	*/
unsigned char TimerOut(unsigned short nto)
{
	unsigned short to_s_d, *pmaxt;

	to_s_d	 =  nto & CONTEGGIO_IN_SEC  ?  ToSec : Time;
	nto		&= ~CONTEGGIO_IN_SEC;
	pmaxt	 = &MaxTime[nto];

	if (*pmaxt == TIMER_STOP)
	{
		return (FALSE);
	}
	else
	{
		if (*pmaxt == TEMPO_SCADUTO)
		{
			return (TRUE);
		}
		else 
		{
			if ((unsigned short)(to_s_d - CntTime[nto]) >= *pmaxt)
			{
				*pmaxt = TEMPO_SCADUTO;
				return (TRUE);
			}
			else
			{
				return (FALSE);
			}
		}
	}
}


/*	calcolo quanto tempo manca al termine di un time out attivato
	precedentemente; il tempo deve essere calcolato in sec. o nell'unit� base usata a
	seconda del tipo di timer attivato; ritorna 0 se e` gia` passato
	il tempo massimo o comunque il timer ha gia` terminato;
*/
unsigned short QuantoTempoManca(unsigned short nto)
{
	unsigned short to_s_d, maxt, tm;
	to_s_d	 =  nto & CONTEGGIO_IN_SEC  ?  ToSec : Time;
	nto		&= ~CONTEGGIO_IN_SEC;
	maxt	 = MaxTime[nto];
	tm		 = maxt - to_s_d + CntTime[nto];
	return ((maxt == TIMER_STOP || maxt == TEMPO_SCADUTO || tm > maxt) ?  0 : tm);
} 


unsigned short BaseTempi(void)
{
	return (Time);
}


