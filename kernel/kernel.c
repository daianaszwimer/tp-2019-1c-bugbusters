#include "kernel.h"

int main(void) {
	logger_KERNEL = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
	log_info(logger_KERNEL, "----------------INICIO DE KERNEL--------------");

	//	SEMAFOROS
	sem_init(&semLeerDeConsola, 0, 1);
	sem_init(&semEnviarMensajeAMemoria, 0, 0);

	//HILOS
	pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL);

	enviarMensajeAMemoria();

	pthread_join(hiloLeerDeConsola, NULL);

	log_destroy(logger_KERNEL);
	return EXIT_SUCCESS;
}


void leerDeConsola(void){
	//log_info(logger, "leerDeConsola");
	while (1) {
		sem_wait(&semLeerDeConsola);
		mensaje = readline(">");
		if (!strcmp(mensaje, "\0")) {
			sem_post(&semEnviarMensajeAMemoria);
			break;
		}
		codValidacion = validarMensaje(mensaje, KERNEL, logger_KERNEL);
		sem_post(&semEnviarMensajeAMemoria);
	}
}

void enviarMensajeAMemoria(void) {
	//log_info(logger, "enviarMensajeAMemoria");
	int codRequest; // es la palabra reservada (ej: SELECT)
	char** request;
	t_paquete* paquete;
	int conexionMemoria;

	t_config* config = leer_config("/home/utnso/tp-2019-1c-bugbusters/kernel/kernel.config");
	conexionMemoria = crearConexion(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"));


	while (1) {
		sem_wait(&semEnviarMensajeAMemoria);
		if (!strcmp(mensaje, "\0")) {
			break;
		}
		printf("El mensaje es: %s \n", mensaje);
		printf("Codigo de validacion: %d \n", codValidacion);
		if (codValidacion != EXIT_FAILURE && codValidacion != QUERY_ERROR) {
			request = separarString(mensaje);
			codRequest = obtenerCodigoPalabraReservada(request[0], KERNEL);
			// El paquete tiene el cod_request y UN request completo
			paquete = armar_paquete(codRequest, mensaje);
			printf("Voy a enviar este cod: %d \n", paquete->palabraReservada);
			log_info(logger_KERNEL, "Antes de enviar mensaje");
			enviar(paquete, conexionMemoria);
			free(paquete);
			log_info(logger_KERNEL, "despues de enviar mensaje");
		}		
		free(mensaje);			
		sem_post(&semLeerDeConsola);

	}
	log_destroy(logger);
	liberar_conexion(conexionMemoria);
	config_destroy(config);
}
