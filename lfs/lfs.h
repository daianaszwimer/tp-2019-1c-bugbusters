#ifndef LFS_H_
#define LFS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <nuestro_lib/nuestro_lib.h>
#include <pthread.h>
#include <sys/stat.h>

t_log* logger_LFS;
t_config* config;
t_list* descriptoresClientes;
fd_set descriptoresDeInteres;			// Coleccion de descriptores de interes para select
#define PATH "/home/utnso/tp-2019-1c-bugbusters/lfs"
char* pathRaiz;

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

void* leerDeConsola(void*);
void* recibirMemorias(void*);
void* conectarConMemoria(void*);
void interpretarRequest(cod_request, char*, int);
errorNo procesarCreate(char*, char*, char*, char*);
errorNo procesarInsert(char*, uint16_t, char*, unsigned long long);
errorNo procesarSelect(char*, char*);
int obtenerBloqueDisponible(void);
int crearDirectorio(char*);
int mkdir_p(const char*);
void inicializarLfs(void);
void liberarString(char*);
errorNo crearParticiones(char*, char*);
void* hiloDump();
errorNo dumpear();
void vaciarTabla(t_tabla*);

#endif /* LFS_H_ */
