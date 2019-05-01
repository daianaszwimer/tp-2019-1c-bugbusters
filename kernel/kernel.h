#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <nuestro_lib/nuestro_lib.h>
#include <pthread.h>
#include <semaphore.h>

t_log* logger_KERNEL;
sem_t semLeerDeConsola;				// semaforo para el leer consola
sem_t semEnviarMensajeAMemoria;		// semaforo para enviar mensaje
pthread_t hiloLeerDeConsola;			// hilo que lee de consola
char* mensaje;  					// es el request completo
int codValidacion;

void inicializarVariables(void);
void leerDeConsola(void);
void enviarMensajeAMemoria(void);


#endif /* KERNEL_H_*/
