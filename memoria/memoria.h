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
#include<math.h>
#include <errno.h>


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
	JOURNALTIME = -10103
} t_erroresMemoria;

//-----------------STRUCTS------------------

typedef struct{
	unsigned long long timestamp;
	uint16_t key;
	char value[]; // al inicializarse, lfs me tiene q decir el tamanio
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

typedef struct{
	char* path;
	pthread_mutex_t* sem;
}t_semSegmento;

//-------------VARIABLES GLOBALES-------------------------


t_log* logger_MEMORIA;
t_config* config;
t_handshake_lfs* handshakeLFS;

t_tablaDeSegmentos* tablaDeSegmentos;
t_segmento* tablaA;
t_elemTablaDePaginas* elementoA1;
t_marco* frame0;


sem_t semLeerDeConsola;				// semaforo para el leer consola
sem_t semEnviarMensajeAFileSystem;		// semaforo para enviar mensaje
pthread_mutex_t terminarHilo;
pthread_mutex_t semMBitarray;
pthread_mutex_t semMTablaSegmentos;
pthread_mutex_t semMDescriptores;
pthread_mutex_t semMListSemSegmentos;
t_list* semMPorSegmento;

pthread_t hiloLeerDeConsola;			// hilo que lee de consola
pthread_t hiloEscucharMultiplesClientes;// hilo para escuchar clientes

t_bitarray* bitarray;
char* bitarrayString;
void* memoria;
int marcosTotales;
int marcosUtilizados=0;
int conexionLfs, flagTerminarHiloMultiplesClientes= 0;
int maxValue;

t_list* listaSemSegmentos;
t_list* descriptoresClientes ;
fd_set descriptoresDeInteres;					// Coleccion de descriptores de interes para select
t_list* clientesGossiping;
t_list* clientesRequest;

//------------------ --- FUNCIONES--------------------------------

void conectarAFileSystem(void);
void inicializacionDeMemoria(void);
int obtenerIndiceMarcoDisponible();

void leerDeConsola(void);
void validarRequest(char*);

void escucharMultiplesClientes(void);
void interpretarRequest(int, char*,t_caller, int);
t_paquete* intercambiarConFileSystem(cod_request, char*);

void procesarSelect(cod_request,char*,consistencia, t_caller, int);

int estaEnMemoria(cod_request, char*, t_paquete**, t_elemTablaDePaginas**,char**);
void lockSemSegmento(char*);
void desbloquearSemSegmento(char* );
void enviarAlDestinatarioCorrecto(cod_request, int, char*, t_paquete* , t_caller, int);
void mostrarResultadoPorConsola(cod_request, int,char*,t_paquete* );
void guardarRespuestaDeLFSaCACHE(t_paquete* ,t_erroresMemoria);

void procesarInsert(cod_request, char*,consistencia, t_caller,int);
void insertar(int resultadoCache,cod_request,char*,t_elemTablaDePaginas* ,t_caller, int,char*);
t_paquete* armarPaqueteDeRtaAEnviar(char*);

void actualizarTimestamp(t_marco*);
void actualizarPagina (t_marco*, char*);
void actualizarElementoEnTablaDePagina(t_elemTablaDePaginas*, char* );

t_marco* crearMarcoDePagina(t_marco*,uint16_t, char*, unsigned long long);
t_elemTablaDePaginas* crearElementoEnTablaDePagina(int id,t_marco* ,uint16_t, char*,unsigned long long);
void crearSegmento(t_segmento*,char*);


void procesarCreate(cod_request, char*,consistencia, t_caller, int);
void create(cod_request,char*);
t_erroresMemoria existeSegmentoEnMemoria(cod_request,char*);


int obtenerPaginaDisponible(t_marco**);

void eliminarSegmento(t_segmento*);
void eliminarElemTablaPagina(t_elemTablaDePaginas* );
void eliminarElemTablaSegmentos(t_segmento*);
void removerSem(char*);
void liberarSemSegmento(t_semSegmento*);
void liberarEstructurasMemoria();
void liberarMemoria();
void eliminarMarco(t_elemTablaDePaginas*,t_marco* );
void procesarDescribe(cod_request, char*,t_caller,int);
void procesarDrop(cod_request, char* ,consistencia , t_caller , int);

int desvincularVictimaDeSuSegmento(t_elemTablaDePaginas*);
int menorTimestamp(t_elemTablaDePaginas*,t_elemTablaDePaginas*);
t_elemTablaDePaginas* correrAlgoritmoLRU(int*);

void procesarJournal(cod_request, char*, t_caller, int);
t_list* obtenerTablasModificadas(t_segmento*);
int tablaDePaginaModificada(t_elemTablaDePaginas*);


#endif /* MEMORIA_H_ */
