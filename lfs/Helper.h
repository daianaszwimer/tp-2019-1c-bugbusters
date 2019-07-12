/*
 * Helper.h
 *
 *  Created on: 30 jun. 2019
 *      Author: utnso
 */

#ifndef HELPER_H_
#define HELPER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <nuestro_lib/nuestro_lib.h>
#include <pthread.h>
#include <commons/collections/queue.h>


typedef struct{
	unsigned long long timestamp;
	uint16_t key;
	char* value;
} t_registro;

typedef struct
{
	char* nombreTabla;
	t_list* registros;
} t_tabla;

typedef struct
{
	t_list* tablas;
} t_memtable;

typedef struct{
	pthread_t* thread;
	char* nombreTabla;
	int finalizarCompactacion;
	t_list* cosasABloquear;
} t_hiloTabla;

typedef struct {
	int id;
	pthread_mutex_t mutex;
} t_bloqueo;

typedef struct{
	cod_request cod_request;
	char* parametros;
	int* memoria_fd;
} t_request;

#define PATH "/home/utnso/tp-2019-1c-bugbusters/lfs"

t_log* logger_LFS;
t_config* config;
t_config *configMetadata;
t_bitarray* bitarray;
t_memtable* memtable;
t_list* tablasParaCompactaciones; // lista que tiene las tablas para las compactaciones

pthread_t hiloLeerDeConsola;
pthread_t hiloRecibirMemorias;
pthread_t hiloDumpeo;
pthread_t hiloDeCompactacion;
pthread_t hiloDeInotify;

char* pathConfig;
char* pathRaiz;
char* pathTablas;
char* pathMetadata;
char* pathBloques;
int blocks;

int obtenerBloqueDisponible();
void vaciarTabla(t_tabla*);
void liberarMutexTabla(t_bloqueo*);
void interpretarRequest(cod_request, char*, int*);

// SEMAFOROS
pthread_mutex_t mutexMemtable;
pthread_mutex_t mutexTablasParaCompactaciones;
pthread_mutex_t mutexConfig;
pthread_mutex_t mutexRetardo;
pthread_mutex_t mutexTiempoDump;
pthread_mutex_t mutexBitmap;

//VARIABLES CONFIG
int retardo;
int tiempoDump;
int tamanioValue;

#endif /* HELPER_H_ */
