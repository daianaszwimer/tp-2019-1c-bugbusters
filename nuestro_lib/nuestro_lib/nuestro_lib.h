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

//TODO: poner esto en el config de memoria
#define IP "127.0.0.1"
#define PUERTO "4444"

typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

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
	METRICS
} cod_request;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	cod_request palabraReservada;
	char* request;
} t_paquete;

t_log* logger;

void iterator(char*);
int crearConexion(char*, char*);
t_config* leer_config(char*);
//void leer_consola(t_log* logger);
t_paquete* armar_paquete(cod_request, char*);
//void _leer_consola_haciendo(void(*accion)(char*));
int validarMensaje(char*);
int obtenerCodRequest(char*);

////servidor
//void* recibir_buffer(int*, int);
int iniciar_servidor(void);
int esperar_cliente(int);
//t_list* recibir_paquete(int);
//void recibir_mensaje(int);
//int recibir_operacion(int);
//
////cliente
//void enviar_mensaje(char* mensaje, int socket_cliente);
//t_paquete* crear_paquete(void);
//t_paquete* crear_super_paquete(void);
//void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
//void liberar_conexion(int socket_cliente);
//void eliminar_paquete(t_paquete* paquete);


#endif /* SOCKETS_H_ */
