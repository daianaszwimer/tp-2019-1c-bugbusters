#include "kernel.h"

int main(void) {

	logger = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
	log_info(logger, "----------------INICIO DE KERNEL--------------");
	conectarAMemoria();
	return EXIT_SUCCESS;
}

void conectarAMemoria(){

	// Toda esta primera parte (hasta crearConexion exclusive) podriamos modelarla en una funcion leerDeConsola
	// que va en el main antes de conectarAMemoria

	char* mensaje;   // es el request completo
	int cod_request; // es la palabra reservada (ej: SELECT)
	char** request;
	t_config* config = leer_config("/home/utnso/tp-2019-1c-bugbusters/kernel/kernel.config");

	// En este while se lee de la consola
	while(1) {
		mensaje = readline(">");
		if (strcmp(mensaje,"\n") != 0) {
			break;
		}
	}

	printf("El mensaje es: %s \n", mensaje);
	int codValidacion = validarMensaje(mensaje, KERNEL);
	printf("cod validacon %d \n", codValidacion);
	request = separarString(mensaje);
	cod_request = obtenerCodigoPalabraReservada(request[0], KERNEL);

	printf("Y ahora es: %s \n", mensaje);
	//int conexion = crearConexion(config_get_string_value(config, "IP"), config_get_string_value(config, "PUERTO"));
	int conexion = crearConexion("127.0.0.1","4444");
	// El paquete tiene el cod_request y UN request completo
	t_paquete* paquete = armar_paquete(cod_request, mensaje);
	printf("Voy a enviar este cod: %d \n", paquete->palabraReservada);
	//printf("Y este es el msj: %s \n", (char*) paquete->request);
	//char** primerParametro = (char*) paquete->request;
	//printf("Y este es el 1er parametro: %s \n", primerParametro[1]);
	log_info(logger,"Antes de enviar mensaje");
	enviar(paquete, conexion);

	//eliminar_paquete(paquete);
	//enviar_mensaje(mensaje ,conexion);

//	t_list* lista;
//
//	while(1)
//			{
//				int cod_op = recibir_operacion(conexion);
//				switch(cod_op)
//				{
//				case MENSAJE:
//					recibir_mensaje(conexion);
//					break;
//				case PAQUETE:
//					lista = recibir_paquete(conexion);
//					printf("Me llegaron los siguientes valores:\n");
//					list_iterate(lista, (void*) iterator);
//					//char* msg = string_repeat('f', 3);
//					t_paquete* paquete = armar_paquete();
//					enviar_paquete(paquete, conexion);
//					eliminar_paquete(paquete);
//					//enviar_mensaje("123", cliente_fd);
//					break;
//				case -1:
//					log_error(logger, "el cliente se desconecto. Terminando servidor");
//					break;
//				default:
//					log_warning(logger, "Operacion desconocida. No quieras meter la pata");
//					break;
//				}
//		}
	//-----------------------

  log_destroy(logger);
  free(mensaje);
//	config_destroy(config);		//Preguntar
  close(conexion);

}
