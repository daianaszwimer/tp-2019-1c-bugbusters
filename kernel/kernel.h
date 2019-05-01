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
int conexionMemoria;
t_config* config;

sem_t semLeerDeConsola;				// semaforo para el leer consola
sem_t semEnviarMensajeAMemoria;		// semaforo para enviar mensaje
pthread_t hiloLeerDeConsola;		// hilo que lee de consola
pthread_t hiloConectarAMemoria;		//hilo que conecta a memoria

void conectarAMemoria(void);
void liberarMemoria(void);
void interpretarRequest(char *);
void manejarRequest(char *);
void leerDeConsola(void);
void enviarMensajeAMemoria(cod_request, char*);


#endif /* KERNEL_H_*/
