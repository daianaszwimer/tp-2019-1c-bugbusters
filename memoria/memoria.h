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
	ANOTHER_COMPONENT
} t_caller;

typedef enum
{
	SINMODIFICAR,
	MODIFICADO
} t_flagModificado;

typedef struct{
	unsigned long long timestamp;
	uint16_t key;
	char* value; // al inicializarse, lfs me tiene q decir el tamanio
}t_pagina;

typedef struct{
	int numeroDePag;
	t_pagina* pagina;
	t_flagModificado modificado;
}t_elemTablaDePaginas;

typedef struct{
	t_list* elementosDeTablaDePagina;
}t_tablaDePaginas;

typedef struct{
	time_t tv_sec;
	suseconds_t tv_usec;   /* microseconds */
}t_timeval;

t_log* logger_MEMORIA;
t_config* config;


//
t_tablaDePaginas tablaA;

t_pagina* pag;

t_elemTablaDePaginas* elementoA1;
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

int estaEnMemoria(cod_request, char**, char**, t_elemTablaDePaginas**);
void enviarAlDestinatarioCorrecto(cod_request, char*, char* , t_caller, int);

t_pagina* crearPagina(uint16_t, char*);
void actualizarPagina (t_pagina*, char*);
void agregarElementoATablaDePagina(t_tablaDePaginas*, int,t_pagina*);


#endif /* MEMORIA_H_ */
