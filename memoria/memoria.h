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
#include <sys/time.h>

typedef enum
{
	CONSOLE,
	HIMSELF
} t_caller;

typedef enum
{
	SINMODIFICAR,
	MODIFICADO
} t_flagModificado;
typedef struct{
	long int timesamp;
	int key;
	char* value; // al inicializarse, lfs me tiene q decir el tamanio
}t_pagina;
typedef struct{
	int numeroDePag;
	t_pagina* pagina;
	 t_flagModificado modificado;
}t_tablaDePaginas;
typedef struct{
	time_t tv_sec;
	suseconds_t tv_usec;   /* microseconds */
}t_timeval;

t_log* logger_MEMORIA;
t_config* config;

//t_tablaDePaginas* tablaDePaginas[1];
//
t_pagina* pag;

t_tablaDePaginas* tablaA;



//



sem_t semLeerDeConsola;				// semaforo para el leer consola
sem_t semEnviarMensajeAFileSystem;		// semaforo para enviar mensaje

pthread_t hiloLeerDeConsola;			// hilo que lee de consola
pthread_t hiloEscucharMultiplesClientes;// hilo para escuchar clientes
pthread_t hiloEnviarMensajeAFileSystem;	// hilo para enviar mensaje a file system

int conexionLfs;

t_list* descriptoresClientes ;
bool datoEstaEnCache;
fd_set descriptoresDeInteres;					// Coleccion de descriptores de interes para select


void leerDeConsola(void);
unsigned long long obtenerHoraActual();
void validarRequest(char*);

void escucharMultiplesClientes(void);
void interpretarRequest(cod_request, char*,t_caller, int);
char* intercambiarConFileSystem(cod_request, char*);

void conectarAFileSystem(void);
void procesarSelect(cod_request,char*,t_caller, int);
int estaEnCache(cod_request, char**);
#endif /* MEMORIA_H_ */
