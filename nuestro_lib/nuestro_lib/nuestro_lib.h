//servidor
#ifndef SOCKETS_H_
#define SOCKETS_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<signal.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<string.h>
#include<readline/readline.h>

#define TRUE 1
#define FALSE 0

#define PARAMETROS_SELECT 2
#define PARAMETROS_INSERT 3
#define PARAMETROS_INSERT_TIMESTAMP 4
#define PARAMETROS_CREATE 4
#define PARAMETROS_DESCRIBE_GLOBAL 0
#define PARAMETROS_DESCRIBE_TABLA 1
#define PARAMETROS_DROP 1
#define PARAMETROS_JOURNAL 0
#define PARAMETROS_ADD 4
#define PARAMETROS_RUN 1
#define PARAMETROS_METRICS 0

typedef enum
{
	KERNEL,
	MEMORIA,
	LFS
} Componente;

typedef enum
{
	SC,
	SHC,
	EC,
	CONSISTENCIA_INVALIDA = -1
} consistencia;

typedef enum
{
	SUCCESS,
	TABLA_EXISTE,
	TABLA_NO_EXISTE,
	ERROR_CREANDO_ARCHIVO,
	ERROR_CREANDO_DIRECTORIO,
	ERROR_CREANDO_METADATA,
	ERROR_CREANDO_PARTICIONES,
	KEY_NO_EXISTE
} errorNo;

typedef enum
{
	SELECT,
	INSERT,
	CREATE,
	DESCRIBE,
	DROP,
	JOURNAL,
	ADD,
	RUN,
	METRICS,
	NUESTRO_ERROR = -1,
	SALIDA = 404
} cod_request;

typedef struct
{
	int palabraReservada;
	int tamanio;
	char* request;
} t_paquete;

typedef struct
{
	char* puertos;
	char* ips;
	int tamanioIps;
	int tamanioPuertos;
} t_handshake_memoria;

typedef enum
{
	CONSOLE,
	ANOTHER_COMPONENT
} t_caller;

int convertirKey(char*);
void convertirTimestamp(char*, unsigned long long*);
void iterator(char*);

char** separarRequest(char*);

unsigned long long obtenerHoraActual();
char** separarString(char*);
int longitudDeArrayDeStrings(char**);
char** obtenerParametros(char*);
int longitudDeArrayDeStrings(char**);


int crearConexion(char*, char*);
t_config* leer_config(char*);

t_paquete* armar_paquete(int, char*);

int validarMensaje(char*, Componente, t_log*);
int cantDeParametrosEsCorrecta(int,int);
int validarPalabraReservada(int,Componente, t_log*);
int validadCantDeParametros(int, int, t_log*);
int obtenerCodigoPalabraReservada(char*, Componente);

int validarValue(char*,char*, int, t_log*);

////servidor

void* recibir_buffer(int*, int);
int iniciar_servidor(char*, char*);
int esperar_cliente(int);
t_paquete* recibir(int);
t_handshake_memoria* recibirHandshakeMemoria(int);

////cliente

void* serializar_handshake_memoria(t_handshake_memoria*, int);
void* serializar_paquete(t_paquete* , int);
void enviar(cod_request, char*, int);
void enviarHandshakeMemoria(char*, char*, int);
void eliminar_paquete(t_paquete*);
void liberar_conexion(int);
void liberarArrayDeChar(char**);

/* Multiplexacion */
void eliminarClientesCerrados(t_list*, int*);
int maximo(t_list*, int, int);

#endif /* SOCKETS_H_ */
