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

t_log* logger_LFS;
t_config* config;

pthread_t hiloLeerDeConsola;			// hilo que lee de consola

char* mensaje;
int codValidacion;

void recibirConexionMemoria(void);
void leerDeConsola(void);

#endif /* LFS_H_ */
