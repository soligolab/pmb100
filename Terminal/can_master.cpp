
#include "stdint.h"
#include "candef.h"
#include "def.h"
#include "ptype.h"
#include "var.h"

#include <linux/can.h>
#include <linux/can/raw.h>

#include <iostream>

#include <pthread.h>
#include <stdio.h>
#include <util.h>

extern void dmesgDebug(const char *msg1, const char *msg2);

int RunTaskCan=1;

bool PLE500INDEX;
void *th_timer(void * arg) //task 10ms
{
	(void)arg;
	while (RunTaskCan)
	{
		GestioneTimer();
		msleep(10);
	}
	pthread_exit(0);
}

void *th_can(void * arg) //task 1ms
{
	(void)arg;
	while (RunTaskCan)
	{
		RicezioneCanMsg();
		GestioneTrasmissioneMasterCanOpen();
		RicezioneCanMsg();
		//StartTransmission();
		msleep(1);
	}
	pthread_exit(0);
}

int GestioneNumerazioneNodiCan(void)
{
	dmesgDebug("PLE500 Index START", "");
	//touch("/tmp/PLE500Index");
	RunTaskCan = 1;
	//base tempi dei timer
	pthread_t th1, th2;
	InizializzaTimer(); //	Inizializza i timer software
	int status = pthread_create(&th1, NULL, th_timer, NULL);
	if (status < 0)
	{
		//fprintf(stderr, "pthread_create th1 error \n");
		exit(1);
	}
	
	ModalitaCan = MASTER;
	HW_InitCan();
	InizializzaReteCanOpen();
	status = pthread_create(&th2, NULL, th_can, NULL);
	if (status < 0)
	{
		//fprintf(stderr, "pthread_create th2 error \n");
		exit(1);
	}
	
	while(ModalitaCan == MASTER)
		msleep(200);
	
	HW_ClosetCan();
	RunTaskCan = 0;
	
	if (PLE500INDEX == false)
	{
#ifdef DEBUG
		if (ModalitaCan == MASTER)
			printf("!! Errore Numerazione !! TEMPO_RITARDO_AVVIO_MASTER_CAN scaduto\n");
		else
			printf("!! Numerazione OK Numero EPL Trovati:%d  Tentativi:%d !!\n", ArrayImpianto.NumeroNodiRilevati, ArrayImpianto.TentativiNumerazione);	  
#endif // DEBUG

		if (ArrayImpianto.NumeroNodiRilevati == 0)
		{
			system("ip link set down can0");
			dmesgDebug("PLE500 Index: End, Nessun PLE presente ***","");
		}
		else
		{
			char str[4];
			snprintf(str, sizeof(str), "%d", ArrayImpianto.NumeroNodiRilevati);
			dmesgDebug("PLE500 End Index: PLE Presenti n=", str);
			touch("/tmp/PLE500present");
		}
	}
	else
	{
		if (ArrayImpianto.NumeroNodiRilevati == 0)
		{
			system("ip link set down can0");
			dmesgDebug("PLE500 Index: End, Nessun PLE presente ***","");
		}
		else
		{
			char str[4];
			snprintf(str, sizeof(str), "%d", ArrayImpianto.NumeroNodiRilevati);
			dmesgDebug("PLE500 End Index: PLE Presenti n=",str);
			touch("/tmp/PLE500present");
		}
	}
	return ArrayImpianto.NumeroNodiRilevati;
}


void InizializzaReteCanOpen(void)
{
	ArrayImpianto.StepNumerazioneRete = START_NUMERAZIONE;
	ArrayImpianto.NumeroNodiRilevati = 0;
	ArrayImpianto.TentativiNumerazione = 0;
	msleep(100);
}


char SequenzaNumerazione(void)
{
	if (ArrayImpianto.TimerNumerazione)
	{
		ArrayImpianto.TimerNumerazione--;
	}
	else
	{
		//	se TimerNumerazione scaduto
		if (ArrayImpianto.StepNumerazioneRete == NUMERAZIONE_COMPLETATA)
		{
			return (TRUE);
		}
		else
		{
			if (ArrayImpianto.StepNumerazioneRete == INVIO_RESET_USCITA_NUMERAZIONE)
			{
				ArrayImpianto.StepNumerazioneRete = NUMERAZIONE_COMPLETATA;

				TrasmettiComandoNMT(RESET_NODE, 0); //	trasmette comando di RESET a tutti i nodi

				//	procedura di numerazione completata, passa il can in modalit� slave "ridotta" a servizio della procedura di aggiornamento firmware
				ModalitaCan = SLAVE;
				StatoCanOpen = INIT_CAN;
				RitardoInitCan = TEMPO_BOOT_UP_DEFAULT;
			}
			else
			{
				if (ArrayImpianto.StepNumerazioneRete == START_NUMERAZIONE)
				{
					//	invia il comando di start numerazione a tutti i nodi presenti, in modo che si possano
					//	mettere nella condizione di acquisizione numero di nodo.
					if (BufferTrasmissioneLibero())
					{
						TrasmettiComandoNumerazione(COMANDO_NUMERAZIONE_NODO, 0); //	trasmette comando di start procedura di numerazione a tutti i dispositivi collegati
						TimerOn(TIMER_MAX_ATTESA_BUFFER_LIBERO, TEMPO_MAX_ATTESA_BUFFER_LIBERO);

						ArrayImpianto.TentativiNumerazione++;
						if (ArrayImpianto.TentativiNumerazione >= NUMERO_INVII_CODICE_START_NUMERAZIONE)
						{
							//	passa allo step successivo della numerazione
							ArrayImpianto.StepNumerazioneRete++;
							//	azzera il numero di tentativi per usarlo nel prossimo step della numerazione
							ArrayImpianto.TentativiNumerazione = 0;
						}
						//MICRO_ID_OUT_ON;
						ArrayImpianto.TimerNumerazione = RITARDO_COMANDO_INIZIA_NUMERAZIONE;
						ArrayImpianto.NumeroNodiRilevati = 0;
						ArrayImpianto.SlaveAddressInNumerazione = 1;
						ArrayImpianto.AttesaRispostaNumerazione = FALSE;
						ArrayImpianto.AttesaRispostaAttivazioneUscita = FALSE;
					}
				}
				else
				{
					if (((ArrayImpianto.StepNumerazioneRete - SET_ADDRESS_NODO_1) % 4) == 0)
					{
						//	Entra se ArrayImpianto.StepNumerazioneRete = SET_ADDRESS_NODO_1...SET_ADDRESS_NODO_126
						if (BufferTrasmissioneLibero())
						{
							//	trasmette comando di numerazione al nodo con MICRO_ID_IN = 1 e non ancora numerato
							TrasmettiComandoNumerazione(COMANDO_NUMERAZIONE_NODO, ArrayImpianto.SlaveAddressInNumerazione);
							TimerOn(TIMER_MAX_ATTESA_BUFFER_LIBERO, TEMPO_MAX_ATTESA_BUFFER_LIBERO);

							ArrayImpianto.StepNumerazioneRete++;
							ArrayImpianto.TimerNumerazione = RITARDO_ATTESA_RISPOSTA_NUMERAZIONE_NODO;
							ArrayImpianto.AttesaRispostaNumerazione = TRUE;
						}
						else
						{
							//	Buffer di trasmissione occupato, il messaggio precedente non esce.
							if (TimerOut(TIMER_MAX_ATTESA_BUFFER_LIBERO))
							{
								//	se messaggio nel buffer non esce, significa nessuna espansione presente sul bus.
								//	esce dalla procedura di numerazione
								//MICRO_ID_OUT_OFF;
								ArrayImpianto.StepNumerazioneRete = NUMERAZIONE_COMPLETATA;

								//	procedura di numerazione completata, passa il can in modalit� slave "ridotta" a servizio della procedura di aggiornamento firmware
								ModalitaCan = SLAVE;
								StatoCanOpen = INIT_CAN;
								RitardoInitCan = TEMPO_BOOT_UP_DEFAULT;
							}
						}
					}
					else
					{
						if (((ArrayImpianto.StepNumerazioneRete - ATTESA_NUMERAZIONE_NODO_1) % 4) == 0)
						{
							//	Entra se ArrayImpianto.StepNumerazioneRete = ATTESA_NUMERAZIONE_NODO_1...ATTESA_NUMERAZIONE_NODO_126
							//	se arriva qui, significa che non � arrivata in tempo la risposta del nodo al
							//	comando di numerazione, oppure il nodo che si sta tentando di numerare non esiste.
							ArrayImpianto.AttesaRispostaNumerazione = FALSE;
							ArrayImpianto.TentativiNumerazione++;
							if (ArrayImpianto.TentativiNumerazione < MAX_NUMERO_TENTATIVI_NUMERAZIONE_NODO)
								ArrayImpianto.StepNumerazioneRete--; //	reinvia il comando di numerazione
							else
							{
								//	se falliti MAX_NUMERO_TENTATIVI_NUMERAZIONE_NODO significa che i nodi sono terminati
	//							ArrayImpianto.StepNumerazioneRete = NUMERAZIONE_COMPLETATA;
								//MICRO_ID_OUT_OFF;
								ArrayImpianto.StepNumerazioneRete = INVIO_RESET_USCITA_NUMERAZIONE;
								ArrayImpianto.TimerNumerazione = RITARDO_INVIO_RESET_FINE_NUMERAZIONE;


								//							TrasmettiComandoNMT(RESET_NODE, 0);		//	trasmette comando di RESET a tutti i nodi
								//
								//							//	procedura di numerazione completata, passa il can in modalit� slave "ridotta" a servizio della procedura di aggiornamento firmware
								//							ModalitaCan = SLAVE;
								//							StatoCanOpen = INIT_CAN;
								//							RitardoInitCan = TEMPO_BOOT_UP_DEFAULT;
							}
						}
						else
						{
							if (((ArrayImpianto.StepNumerazioneRete - SET_MICRO_ID_OUT_NODO_1) % 4) == 0)
							{
								//	Entra se ArrayImpianto.StepNumerazioneRete = SET_MICRO_ID_OUT_NODO_1...SET_MICRO_ID_OUT_NODO_126
								//	se arriva qui significa che il nodo che � stato numerato ha inviato la sua risposta.
								if (BufferTrasmissioneLibero())
								{
									//	trasmette al nodo appena numerato, il comando per disattivare la sua uscita MICRO_ID_OUT
									TrasmettiComandoNumerazione(COMANDO_DISATTIVA_MICRO_ID_OUT, ArrayImpianto.SlaveAddressInNumerazione);
									ArrayImpianto.StepNumerazioneRete++;
									ArrayImpianto.TimerNumerazione = RITARDO_STABILIZZA_USCITA_NUMERAZIONE;
									ArrayImpianto.AttesaRispostaAttivazioneUscita = TRUE;
								}
							}
							else
							{
								if (((ArrayImpianto.StepNumerazioneRete - ATTESA_SET_MICRO_ID_OUT_NODO_1) % 4) == 0)
								{
									//	Entra se ArrayImpianto.StepNumerazioneRete = ATTESA_SET_MICRO_ID_OUT_NODO_1...ATTESA_SET_MICRO_ID_OUT_NODO_126
									//	se arriva qui, significa che non � arrivata in tempo la risposta del nodo al
									//	comando di disattivazione dell'uscita
									ArrayImpianto.AttesaRispostaAttivazioneUscita = FALSE;
									ArrayImpianto.TentativiNumerazione++;
									if (ArrayImpianto.TentativiNumerazione < MAX_NUMERO_TENTATIVI_ATTIVAZIONE_ID_OUT)
									{
										ArrayImpianto.StepNumerazioneRete--; //	reinvia il comando di numerazione
									}
									else
									{
										//	se falliti MAX_NUMERO_TENTATIVI_ATTIVAZIONE_ID_OUT non si riesce a completare la numerazione
										ArrayImpianto.StepNumerazioneRete = NUMERAZIONE_FALLITA;
										//MICRO_ID_OUT_OFF;
										TrasmettiComandoNMT(RESET_NODE, 0); //	trasmette comando di RESET a tutti i nodi
									}
								}
								else
								{
									if (ArrayImpianto.StepNumerazioneRete == NUMERAZIONE_FALLITA)
									{
										//	procedura di numerazione completata, passa il can in modalit� slave "ridotta" a servizio della procedura di aggiornamento firmware
										ModalitaCan = SLAVE;
										StatoCanOpen = INIT_CAN;
										RitardoInitCan = TEMPO_BOOT_UP_DEFAULT;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return (FALSE);
}


void TrasmettiComandoNumerazione(unsigned char Code, unsigned char Slave)
{
	byte i;
	byte StringaInviata = FALSE;


	for (i = 0; ((i < N_BUFFER_TX) && (StringaInviata == FALSE)); i++)
	{
		if (IntBuffTxCan[BUFFER_OCCUPATO][i] == FALSE)
		{
			IntBuffTxCan[BUFFER_OCCUPATO][i] = TRUE;

			IntBuffTxCan[T_IDR0][i] = NMT;
			IntBuffTxCan[T_IDR1][i] = 0;

			IntBuffTxCan[T_DSR0][i] = Code; //	comando non stardard per gestire la numerazione
			IntBuffTxCan[T_DSR1][i] = Slave;

			IntBuffTxCan[T_DLR][i]	= 2;
			if (++ContatorePrioritaLow > 200)
			{
				ContatorePrioritaLow = 0;
			}
			IntBuffTxCan[T_TBPR][i]	= (ContatorePrioritaLow + 20);
			StringaInviata = TRUE;

			IntBuffTxCan[ESEGUI_TRASMISSIONE][i] = TRUE;
		}
	}
	StartTransmission();
}


/****************************************************************************************************************************/
/*	Questa funzione esegue la gestione della trasmissione di tutti i pacchetti CAN per l'inizializzazione 					*/
/*	e la gestione dei vari moduli di I/O																					*/
/****************************************************************************************************************************/

void GestioneTrasmissioneMasterCanOpen(void)
{
	if (SequenzaNumerazione() == FALSE)
	{
		//	procedura di numerazione ancora in corso...
		return;
	}
}


char BufferTrasmissioneLibero(void)
{
	byte i;
	char BufferLibero = FALSE;

	for (i = 0; ((i < N_BUFFER_TX) && (BufferLibero == FALSE)); i++)
	{
		if (IntBuffTxCan[BUFFER_OCCUPATO][i] == FALSE)
		{
			BufferLibero = TRUE;
		}
	}
	return (BufferLibero);
}


void TrasmettiComandoNMT(unsigned char CommandSpecifier, unsigned char NSlave)
{
	byte i;
	byte StringaInviata = FALSE;


	for (i = 0; ((i < N_BUFFER_TX) && (StringaInviata == FALSE)); i++)
	{
		if (IntBuffTxCan[BUFFER_OCCUPATO][i] == FALSE)
		{
			IntBuffTxCan[BUFFER_OCCUPATO][i] = TRUE;

			IntBuffTxCan[T_IDR0][i] = NMT;
			IntBuffTxCan[T_IDR1][i] = 0;

			IntBuffTxCan[T_DSR0][i] = CommandSpecifier;
			IntBuffTxCan[T_DSR1][i] = NSlave;

			IntBuffTxCan[T_DLR][i]	= 2;
			if (++ContatorePrioritaLow > 200)
			{
				ContatorePrioritaLow = 0;
			}
			IntBuffTxCan[T_TBPR][i]	= (ContatorePrioritaLow + 20);
			StringaInviata = TRUE;

			IntBuffTxCan[ESEGUI_TRASMISSIONE][i] = TRUE;
		}
	}
	StartTransmission();
}


void GestioneInterruptRxMasterCanOpen(void)
{
	byte Slave;
	byte OggettoCanOpen;

	OggettoCanOpen = (IntBuffRxCan[R_IDR0] & 0xF0);
	Slave = (((IntBuffRxCan[R_IDR0] << 3) & 0x78) + ((IntBuffRxCan[R_IDR1] >> 5) & 0x07));

	if ((ArrayImpianto.StepNumerazioneRete != NUMERAZIONE_COMPLETATA) && (ArrayImpianto.StepNumerazioneRete != NUMERAZIONE_FALLITA))
	{
		//	Gestione ricezione durante la procedura di numerazione

		if (Slave >= ADDRESS_PRIMO_NODO_SLAVE_RETE_PLC && Slave <= ADDRESS_ULTIMO_NODO_SLAVE_RETE_PLC)
		{
			//accetta messaggi solo dagli indirizzi slave possibili per la rete del plc

			switch (OggettoCanOpen)
			{
			case NMT_ERROR_CTRL	:	//	gestione messaggio NMT durante la FASE DI NUMERAZIONE

				if ((IntBuffRxCan[R_DSR0] & 0x7F) == STOPPED)
				{
					//	se nodo in STOP
					if (Slave == ArrayImpianto.SlaveAddressInNumerazione && ArrayImpianto.AttesaRispostaNumerazione == TRUE)
					{
						// se arrivata risposta dal nodo appena numerato
						ArrayImpianto.AttesaRispostaNumerazione = FALSE;
						//	passa allo step successivo della procedura di numerazione
						ArrayImpianto.StepNumerazioneRete++;
						//	ricarica il tempo per una nuova trasmissione del master
						ArrayImpianto.TimerNumerazione = RITARDO_COMANDO_ATTIVAZIONE_USCITA;
						//	azzera il numero di tentativi di invio del comando
						ArrayImpianto.TentativiNumerazione = 0;
					}
					//	gestione messaggio NMT durante la FASE DI DISATTIVAZIONE DELL'USCITA MICRO ID OUT
					if (Slave == ArrayImpianto.SlaveAddressInNumerazione && ArrayImpianto.AttesaRispostaAttivazioneUscita == TRUE)
					{
						// se arrivata risposta dal nodo che ha disattivato l'uscita ID_OUT
						ArrayImpianto.AttesaRispostaAttivazioneUscita = FALSE;
						//	passa allo step successivo della procedura di numerazione
						ArrayImpianto.StepNumerazioneRete++;
						//	azzera il numero di tentativi di invio del comando
						ArrayImpianto.TentativiNumerazione = 0;
						//	incrementa il numero di nodi rilevati
						ArrayImpianto.NumeroNodiRilevati++;
						ArrayImpianto.SlaveAddressInNumerazione++;
					}
				}
				break;

			default				:	break;
			}
		}
	}
}

