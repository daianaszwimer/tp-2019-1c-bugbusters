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

//typedef struct{
//	int valor;
//} t_int;

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
	int flag;
} t_hiloTabla;

#define PATH "/home/utnso/tp-2019-1c-bugbusters/lfs"

t_log* logger_LFS;
t_config* config;
t_config *configMetadata;
t_bitarray* bitarray;
t_memtable* memtable;
t_list* listaDeTablas;
pthread_t hiloDeCompactacion;

char* pathRaiz;
char* pathTablas;
char* pathMetadata;
char* pathBloques;
int blocks;

int obtenerBloqueDisponible();
void vaciarTabla(t_tabla*);

#endif /* HELPER_H_ */
