#ifndef LFS_H_
#define LFS_H_

#include "Helper.h"
#include <readline/readline.h>
#include "API.h"
#include "Compactador.h"
#include "Dump.h"
#include <sys/inotify.h>

// Variables para inotify
#define EVENT_SIZE  ( sizeof (struct inotify_event) + 24 )
#define BUF_LEN     ( 1024 * EVENT_SIZE )
int file_descriptor;
int watch_descriptor;

char* bitmap;
int bitmapDescriptor;

void inicializacionLissandraFileSystem();
void crearFSMetadata(char*, char*);
void levantarFS();
void crearFS(char*, char*);
void crearBloques();
void liberarMemoriaLFS();
void* leerDeConsola(void*);
void* recibirMemorias(void*);
void* conectarConMemoria(void*);
int estaBloqueada(char*);
void encolarRequest(char*, cod_request, char*, int*);
void* escucharCambiosEnConfig();
#endif /* LFS_H_ */
