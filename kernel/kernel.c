#include "kernel.h"

int main(void) {

	logger = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
	log_info(logger, "----------------INICIO DE KERNEL--------------");
	conectarAMemoria();
	return EXIT_SUCCESS;
}

void conectarAMemoria() {

	// Toda esta primera parte (hasta crearConexion exclusive) podriamos modelarla en una funcion leerDeConsola
	// que va en el main antes de conectarAMemoria

	char* mensaje;   // es el request completo
	int cod_request; // es la palabra reservada (ej: SELECT)
	int codValidacion;
	char** request;
	int conexion;

	t_config* config = leer_config("/home/utnso/tp-2019-1c-bugbusters/kernel/kernel.config");
	//int conexion = crearConexion(config_get_string_value(config, "IP"), config_get_string_value(config, "PUERTO"));
	conexion = crearConexion("127.0.0.1", "4444");

	while (1) {

		// En este while se lee de la consola
		while (1) {
			mensaje = readline(">");
			if (strcmp(mensaje, "\n") != 0) {
				break;
			}
		}

		printf("El mensaje es: %s \n", mensaje);
		codValidacion = validarMensaje(mensaje, KERNEL);
		printf("COD VALIDACION: %d \n", codValidacion);
		if (codValidacion == EXIT_FAILURE) {
			log_error(logger, "Request invalido"); //ANALIZAR SI DEBERIAMOS LANZAR ERROR O SIMPLEMENTE INFORMLO Y VOLVER  UN ETSADO CONSISTENTE
		} else {
			printf("cod validacion %d \n", codValidacion);
			request = separarString(mensaje);
			cod_request = obtenerCodigoPalabraReservada(request[0], KERNEL);

			printf("Y ahora es: %s \n", mensaje);
			t_paquete* paquete = armar_paquete(cod_request, mensaje);
			printf("Voy a enviar este cod: %d \n", paquete->palabraReservada);
			log_info(logger, "Antes de enviar mensaje");
			enviar(paquete, conexion);
			log_info(logger, "despues de enviar mensaje");

		}


		log_destroy(logger);
		free(mensaje);
		config_destroy(config);
	}
	close(conexion);

}
