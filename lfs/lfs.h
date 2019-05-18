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

pthread_t hiloRecibirDeMemoria;			// hilo que lee de consola

char* mensaje;
int codValidacion;

void leerDeConsola(void);
void recibirConexionesMemoria(void);
void validarRequest(char*);
void procesarRequest(int);
void interpretarRequest(cod_request, char*, t_caller);
void create(char*, char*, int, int);
int obtenerBloqueDisponible(void);
int crearDirectorio(char*);
int mkdir_p(const char*);
void inicializarLfs(void);

#endif /* LFS_H_ */
