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


sem_t semLeerDeConsola;	// semaforo para el leer consola
sem_t semEnviarMensaje;	// semaforo para enviar mensaje
pthread_t hiloLeerConsola;			// hilo que lee de consola
int conexion;
char* mensaje;  					// es el request completo

void inicializarVariables(void);
void leerDeConsola(void);
void enviarMensajeAMemoria(void);

#endif /* KERNEL_H_*/
