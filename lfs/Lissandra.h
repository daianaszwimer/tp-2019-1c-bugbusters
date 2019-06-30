#ifndef LFS_H_
#define LFS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <readline/readline.h>
#include <nuestro_lib/nuestro_lib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "API.h"
#include "Dump.h"

t_log* logger_LFS;
t_config* config;
t_config *configMetadata;
t_list* descriptoresClientes;
fd_set descriptoresDeInteres;			// Coleccion de descriptores de interes para select
#define PATH "/home/utnso/tp-2019-1c-bugbusters/lfs"
char* pathRaiz;
char* pathTablas;
char* pathMetadata;
char* pathBloques;
int blocks;

typedef struct{
	int valor;
} t_int;

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

t_memtable* memtable;
pthread_t hiloLeerDeConsola;
pthread_t hiloRecibirMemorias;
pthread_t hiloDumpeo;


char* mensaje;
int codValidacion;
char* bitmap;
t_bitarray* bitarray;
int bitmapDescriptor;

void inicializacionLissandraFileSystem(char**);
void crearFSMetadata(char*, char*);
void levantarFS();
void crearBloques();
void liberarMemoriaLFS();
void* leerDeConsola(void*);
void* recibirMemorias(void*);
void* conectarConMemoria(void*);
void interpretarRequest(cod_request, char*, int*);
int obtenerBloqueDisponible(errorNo*);
int crearDirectorio(char*);
void crearFS(char*, char*);
errorNo crearParticiones(char*, int);
void vaciarTabla(t_tabla*);
char* obtenerMetadata(char*);

#endif /* LFS_H_ */
