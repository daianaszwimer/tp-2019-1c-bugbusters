#ifndef LFS_H_
#define LFS_H_

#include "Helper.h"
#include <readline/readline.h>
#include "API.h"
#include "Compactador.h"
#include "Dump.h"

pthread_t hiloLeerDeConsola;
pthread_t hiloRecibirMemorias;
pthread_t hiloDumpeo;

char* bitmap;
int bitmapDescriptor;

void inicializacionLissandraFileSystem(char**);
void crearFSMetadata(char*, char*);
void levantarFS();
void crearFS(char*, char*);
void crearBloques();
void liberarMemoriaLFS();
void* leerDeConsola(void*);
void* recibirMemorias(void*);
void* conectarConMemoria(void*);
void interpretarRequest(cod_request, char*, int*);
#endif /* LFS_H_ */
