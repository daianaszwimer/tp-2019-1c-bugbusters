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
#include <commons/bitarray.h>

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
	SEGMENTOEXISTENTE = 99,
	SEGMENTOINEXISTENTE = 100,
	KEYINEXISTENTE =101,
	MEMORIAFULL =-10102
} t_erroresMemoria;

//-----------------STRUCTS------------------
//int maxValue = 20;

typedef struct{
	unsigned long long timestamp;
	uint16_t key;
	char value[20]; // al inicializarse, lfs me tiene q decir el tamanio
}t_marco;

typedef struct{
	int numeroDePag;
	t_marco* marco;
	t_flagModificado modificado;
}t_elemTablaDePaginas;

typedef struct{
	char* path;
	t_list* tablaDePagina;
}t_segmento;

typedef struct{
	t_list* segmentos;
}t_tablaDeSegmentos;


typedef struct{
	time_t tv_sec;
	suseconds_t tv_usec;   /* microseconds */
}t_timeval;


//-------------VARIABLES GLOBALES-------------------------

t_log* logger_MEMORIA;
t_config* config;
t_handshake_lfs* handshake;

t_tablaDeSegmentos* tablaDeSegmentos;
t_segmento* tablaA;
t_elemTablaDePaginas* elementoA1;
t_marco* frame0;


sem_t semLeerDeConsola;				// semaforo para el leer consola
sem_t semEnviarMensajeAFileSystem;		// semaforo para enviar mensaje
pthread_mutex_t terminarHilo;

pthread_t hiloLeerDeConsola;			// hilo que lee de consola
//pthread_attr_t attr;
pthread_t hiloEscucharMultiplesClientes;// hilo para escuchar clientes

t_bitarray* bitarray;
char* bitarrayString;
void* memoria;
int marcosTotales;
int marcosUtilizados=0;
int conexionLfs, flagTerminarHiloMultiplesClientes= 0;

t_list* descriptoresClientes ;
fd_set descriptoresDeInteres;					// Coleccion de descriptores de interes para select


//------------------ --- FUNCIONES--------------------------------

void conectarAFileSystem(void);
void inicializacionDeMemoria(void);
int obtenerIndiceMarcoDisponible();

void leerDeConsola(void);
int validarRequest(char*);

void escucharMultiplesClientes(void);
void interpretarRequest(int, char*,t_caller, int);
t_paquete* intercambiarConFileSystem(cod_request, char*);

void procesarSelect(cod_request,char*,consistencia, t_caller, int);

int estaEnMemoria(cod_request, char*, t_paquete**, t_elemTablaDePaginas**);
void enviarAlDestinatarioCorrecto(cod_request, int, char*, t_paquete* , t_caller, int);
void mostrarResultadoPorConsola(cod_request, int,char*,t_paquete* );
void guardarRespuestaDeLFSaCACHE(t_paquete*,t_erroresMemoria);

void procesarInsert(cod_request, char*,consistencia, t_caller,int);
void insertar(int resultadoCache,cod_request,char*,t_elemTablaDePaginas* ,t_caller, int);
t_paquete* armarPaqueteDeRtaAEnviar(char*);

void actualizarPagina (t_marco*, char*);
void actualizarElementoEnTablaDePagina(t_elemTablaDePaginas*, char* );

t_marco* crearPagina(t_marco*,uint16_t, char*, unsigned long long);
t_elemTablaDePaginas* crearElementoEnTablaDePagina(int id,t_marco* ,uint16_t, char*,unsigned long long);
t_segmento* crearSegmento(char*);


void procesarCreate(cod_request, char*,consistencia, t_caller, int);
void create(cod_request,char*);
t_erroresMemoria existeSegmentoEnMemoria(cod_request,char*);

//void liberarElementoDePag(t_elemTablaDePaginas* self);

void eliminarElemTablaDePaginas(t_elemTablaDePaginas*);
//void eliminarPagina(t_pagina*);

int obtenerPaginaDisponible(t_marco**);

void eliminarElemTablaSegmentos(t_segmento*);
void liberarEstructurasMemoria(t_tablaDeSegmentos*);
void liberarMemoria();
void eliminarMarco(t_elemTablaDePaginas*,t_marco* );
void procesarDescribe(cod_request, char*,t_caller,int);



#endif /* MEMORIA_H_ */
