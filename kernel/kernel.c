#include "kernel.h"
t_log* logger_KERNEL;
int main(void) {

	logger_KERNEL = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
	log_info(logger_KERNEL, "----------------INICIO DE KERNEL--------------");
	conectarAMemoria();
	return EXIT_SUCCESS;
}

void conectarAMemoria() {

	// Toda esta primera parte (hasta crearConexion exclusive) podriamos modelarla en una funcion leerDeConsola
	// que va en el main antes de conectarAMemoria

	char* mensaje;   // es el request completo
	int cod_request; // es la palabra reservada (ej: SELECT)
	char** request;
	int codValidacion;
	int conexion;

	t_config* config = leer_config("/home/utnso/tp-2019-1c-bugbusters/kernel/kernel.config");
	conexion = crearConexion(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"));

	while (1) {

		// En este while se lee de la consola
		while (1) {
			mensaje = readline(">");
			if (strcmp(mensaje, "\n") != 0) {
				break;
			}
		}

		printf("El mensaje es: %s \n", mensaje);
		codValidacion = validarMensaje(mensaje, KERNEL, logger_KERNEL);
		printf("COD VALIDACION: %d \n", codValidacion);
		if (codValidacion == EXIT_FAILURE) {
			log_error(logger_KERNEL, "Request invalido"); //ANALIZAR SI DEBERIAMOS LANZAR ERROR O SIMPLEMENTE INFORMLO Y VOLVER  UN ETSADO CONSISTENTE
		} else {
			printf("cod validacion %d \n", codValidacion);
			request = separarString(mensaje);
			cod_request = obtenerCodigoPalabraReservada(request[0], KERNEL);

			printf("Y ahora es: %s \n", mensaje);

			// El paquete tiene el cod_request y UN request completo
			t_paquete* paquete = armar_paquete(cod_request, mensaje);
			printf("Voy a enviar este cod: %d \n", paquete->palabraReservada);
			log_info(logger_KERNEL, "Antes de enviar mensaje");
			enviar(paquete, conexion);
			log_info(logger_KERNEL, "despues de enviar mensaje");
		}

		log_destroy(logger_KERNEL);
		free(mensaje);
		//config_destroy(config);
	}
	close(conexion);

}
