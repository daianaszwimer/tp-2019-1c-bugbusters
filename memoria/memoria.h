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

pthread_t hiloLeerDeConsola;			// hilo que lee de consola
pthread_t hiloEscucharMultiplesClientes;// hilo que lee de consola
int conexion;
char* mensaje;  					// es el request completo

void leerDeConsola(void);
void escucharMultiplesClientes(void);

t_log* logger_MEMORIA;
t_config* config;
int conexionLfs;
t_list* descriptoresClientes ;
bool datoEstaEnCache;
fd_set descriptoresDeInteres;					// Coleccion de descriptores de interes para select


void interpretarRequest(int, char*, int);
void conectarAFileSystem(void);
void procesarSelect(cod_request, char*);
void recibirConexionKernel(void);

#endif /* MEMORIA_H_ */
