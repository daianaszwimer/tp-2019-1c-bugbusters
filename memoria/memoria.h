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

//---DESCRIPCION FUNCIONALIDADES ACTUALES---
/*funcionalidades actuales de MEMORIA:
(obseracion1: la memoria oseee un atributo en el .h con su consistencia
 observacion2: posee una tablaA 1 "hola")
	1. SELECT
		-SC:SHC: reconoce que debe realizarle la consulta a LFS(pero como aun lisandra no puede hacer un insert, no nos permite hacer nuestro select)
		asi que simplemente loggea que se le consultara a LFS.
		-EV:
			* si se le consulta sobre la tabla y key que posee(observacion2), devuelve correctamente el valor a quien se lo haya consultado(kernel o desde la consola)
			* si se le consulta sobre algo inexistente, al igual que en SC, loggea que se le consulta a LFS
*/

//----------------ENUMS--------------------
typedef enum
{
	SINMODIFICAR,
	MODIFICADO
} t_flagModificado;

typedef enum
{
	SEGMENTOINEXISTENTE = 100,
	KEYINEXISTENTE =101
} t_erroresCache;

//-----------------STRUCTS------------------

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
	char* nombre;
	t_list* elementosDeTablaDePagina;
}t_tablaDePaginas;

typedef struct{
	t_list* segmentos;//TODO por ahora cada segmento es una tablaDePagias pero faltan agregar conceptos
}t_segmentos;
//TODO: revisar teoria para esto ultimo

typedef struct{
	time_t tv_sec;
	suseconds_t tv_usec;   /* microseconds */
}t_timeval;
//-------------CASO DE PRUEBA INSERTAR-------------------------
t_paquete* valorDeLF;


//-------------VARIABLES GLOBALES-------------------------

consistencia consistenciaMemoria = EC;

t_log* logger_MEMORIA;
t_config* config;

t_tablaDePaginas* tablaA;
t_pagina* pag;
t_elemTablaDePaginas* elementoA1;
t_segmentos* tablaDeSegmentos;

sem_t semLeerDeConsola;				// semaforo para el leer consola
sem_t semEnviarMensajeAFileSystem;		// semaforo para enviar mensaje
pthread_mutex_t terminarHilo;

pthread_t hiloLeerDeConsola;			// hilo que lee de consola
pthread_t hiloEnviarMensajeAFileSystem;	// hilo para enviar mensaje a file system
//pthread_attr_t attr;
pthread_t hiloEscucharMultiplesClientes;// hilo para escuchar clientes

int conexionLfs, flagTerminarHiloMultiplesClientes= 0;

t_list* descriptoresClientes ;
fd_set descriptoresDeInteres;					// Coleccion de descriptores de interes para select
//

//------------------ --- FUNCIONES--------------------------------

void leerDeConsola(void);
int validarRequest(char*);

void escucharMultiplesClientes(void);
void interpretarRequest(cod_request, char*,t_caller, int);
t_paquete* intercambiarConFileSystem(cod_request, char*);

void conectarAFileSystem(void);
void procesarSelect(cod_request,char*,t_caller, int);

int estaEnMemoria(cod_request, char*, t_paquete**, t_elemTablaDePaginas**);
void enviarAlDestinatarioCorrecto(cod_request, int, char*, t_paquete* , t_caller, int);
void mostrarResultadoPorConsola(cod_request, int,char*,t_paquete* );
void guardarRespuestaDeLFSaCACHE(t_paquete*,t_erroresCache);

void procesarInsert(cod_request, char*, t_caller,int);
void insertar(int resultadoCache,cod_request,char*,t_elemTablaDePaginas* ,t_caller, int);

int validarInsertSC(errorNo);
t_pagina* crearPagina(uint16_t, char*,unsigned long long);
void actualizarPagina (t_pagina*, char*);

t_elemTablaDePaginas* crearElementoEnTablaDePagina(uint16_t, char*,unsigned long long);
void actualizarElementoEnTablaDePagina(t_elemTablaDePaginas*, char* );

t_tablaDePaginas* crearTablaDePagina(char*);

//void liberarElementoDePag(t_elemTablaDePaginas* self);
void liberarMemoria();



#endif /* MEMORIA_H_ */
