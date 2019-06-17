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
#include <time.h>

typedef struct
{
	char* ip;
	char* puerto;
	char* numero;//todo: es mejor y mas facil si esto es solo un int
} config_memoria;

typedef struct
{
	cod_request codigo;
	void* request;
} request_procesada;

typedef struct
{
	int cantidadSelectInsert;
	int cantidadTotal;
	char* numeroMemoria;
} estadisticaMemoria;

t_log* logger_KERNEL;
t_log* logger_METRICAS_KERNEL;
int conexionMemoria;
int quantum;
t_config* config;
t_queue* new;
t_queue* ready;
config_memoria* memoriaSc;
t_list* memoriasShc;
t_list* memoriasEc;
t_list* memorias;
t_list* tablasSC;
t_list* tablasSHC;
t_list* tablasEC;

// variables de metricas
double tiempoSelectSC = 0.0;
double tiempoInsertSC = 0.0;
int cantidadSelectSC = 0;
int cantidadInsertSC = 0;
t_list* cargaMemoriaSC;
double tiempoSelectSHC = 0.0;
double tiempoInsertSHC = 0.0;
int cantidadSelectSHC = 0;
int cantidadInsertSHC = 0;
t_list* cargaMemoriaSHC;
double tiempoSelectEC = 0.0;
double tiempoInsertEC = 0.0;
int cantidadSelectEC = 0;
int cantidadInsertEC = 0;
t_list* cargaMemoriaEC;

sem_t semRequestNew;				// semaforo para planificar requests en new
pthread_mutex_t semMColaNew;		// semafoto mutex para cola de new
sem_t semRequestReady;				// semaforo para planificar requests en ready
pthread_mutex_t semMColaReady;		// semafoto mutex para cola de ready
sem_t semMultiprocesamiento;		// semaforo contador para limitar requests en exec
pthread_mutex_t semMMetricas;		// semaforo mutex para evitar concurrencia en metricas

pthread_t hiloLeerDeConsola;		// hilo que lee de consola
pthread_t hiloConectarAMemoria;		// hilo que conecta a memoria
pthread_t hiloPlanificarNew;		// hilo para planificar requests de new a ready
pthread_t hiloPlanificarExec;		// hilo para planificar requests de ready a exec y viceversa
pthread_t hiloMetricas;				// hilo para loguear metricas cada 30 secs
pthread_t hiloDescribe;				// hilo para hacer describe cada x secs

void inicializarVariables(void);
void conectarAMemoria(void);
void procesarHandshake(t_handshake_memoria*);
void liberarMemoria(void);
void liberarRequestProcesada(request_procesada*);
void liberarColaRequest(request_procesada*);
void leerDeConsola(void);
void hacerDescribe(void);
void loguearMetricas(void);
void informarMetricas(int);
void liberarEstadisticaMemoria(estadisticaMemoria*);
void aumentarContadores(consistencia, char*, cod_request, double);
// planificar requests
void planificarNewAReady(void);
void planificarReadyAExec(void);
void reservarRecursos(char*);
// validar + delegar requests
int validarRequest(char *);
int manejarRequest(request_procesada*);
// se usa para procesar requests
int enviarMensajeAMemoria(cod_request, char*);
config_memoria* encontrarMemoriaSegunTabla(char*, char*);
void actualizarTablas(char*);
void agregarTablaACriterio(char*);
void liberarTabla(char*);
int encontrarMemoriaPpal(config_memoria*);
consistencia obtenerConsistenciaTabla(char*);
// procesan requests
void procesarJournal(int);
void procesarRun(t_queue*);
int procesarAdd(char*);
void procesarRequest(request_procesada*);

#endif /* KERNEL_H_*/
