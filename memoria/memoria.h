#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <readline/readline.h>
#include <nuestro_lib/nuestro_lib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

t_log* logger_MEMORIA;
t_config* config;

sem_t semLeerDeConsola;				// semaforo para el leer consola
sem_t semEnviarMensajeAFileSystem;		// semaforo para enviar mensaje

pthread_t hiloLeerDeConsola;			// hilo que lee de consola
pthread_t hiloEscucharMultiplesClientes;// hilo para escuchar clientes
pthread_t hiloEnviarMensajeAFileSystem;	// hilo para enviar mensaje a file system
char* mensaje;  					// es el request completo
int conexionLfs;
int codValidacion;
t_list* descriptoresClientes ;
bool datoEstaEnCache;
fd_set descriptoresDeInteres;					// Coleccion de descriptores de interes para select



void leerDeConsola(void);
void escucharMultiplesClientes(void);
void interpretarRequest(int, char*, int);
void enviarMensajeAFileSystem(void);
void conectarAFileSystem(void);
void procesarSelect(cod_request, char*);

#endif /* MEMORIA_H_ */
