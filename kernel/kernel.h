#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <nuestro_lib/nuestro_lib.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct
{
	char* ip;
	char* puerto;
} config_memoria;

t_log* logger_KERNEL;
int conexionMemoria;
t_config* config;
t_queue* new;
t_queue* ready;
t_queue* exec;
config_memoria memoriaSc;

sem_t semRequestNew;				// semaforo para planificar requests en new
sem_t semMColaNew;					// semafoto mutex para cola de new
sem_t semRequestReady;				// semaforo para planificar requests en ready
sem_t semMColaReady;				// semafoto mutex para cola de ready
sem_t semMultiprocesamiento;		// semaforo contador para limitar requests en exec

pthread_t hiloLeerDeConsola;		// hilo que lee de consola
pthread_t hiloConectarAMemoria;		//hilo que conecta a memoria
pthread_t hiloPlanificarNew;		//hilo para planificar requests de new a ready
pthread_t hiloPlanificarExec;		//hilo para planificar requests de ready a exec y viceversa
//todo: hilo que loguea cada x tiempo

void conectarAMemoria(void);
void liberarMemoria(void);
void leerDeConsola(void);
//planificar requests
void planificarNewAReady(void);
void planificarReadyAExec(void);
//validar + delegar requests
int validarRequest(char *);
void manejarRequest(char *);
//funciones que procesan requests:
t_paquete* enviarMensajeAMemoria(cod_request, char*);
void procesarRun(char*);
void procesarAdd(int);
void procesarRequest(char*);


#endif /* KERNEL_H_*/
