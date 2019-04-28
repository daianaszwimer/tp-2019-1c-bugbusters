#include "kernel.h"
t_log* logger_KERNEL;
int main(void) {
	inicializarVariables();
	log_info(logger_KERNEL, "----------------INICIO DE KERNEL--------------");
	sem_init(&semLeerDeConsola, 0, 1);
	sem_init(&semEnviarMensajeAMemoria, 0, 0);


	pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL);


	enviarMensajeAMemoria();

	pthread_join(hiloLeerDeConsola, NULL);

	log_destroy(logger_KERNEL);
	return EXIT_SUCCESS;
}

void inicializarVariables(void){
	logger_KERNEL = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
	//log_info(logger_KERNEL, "InicializarVariables");
	t_config* config = leer_config("/home/utnso/tp-2019-1c-bugbusters/kernel/kernel.config");
	conexion = crearConexion(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"));
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
		sem_post(&semEnviarMensajeAMemoria);
	}
}

void enviarMensajeAMemoria(void) {
	//log_info(logger, "enviarMensajeAMemoria");
	int cod_request; // es la palabra reservada (ej: SELECT)
	char** request;
	int codValidacion;
	t_paquete* paquete;

	while (1) {
		sem_wait(&semEnviarMensajeAMemoria);
		if (!strcmp(mensaje, "\0")) {
			break;
		}
		printf("El mensaje es: %s \n", mensaje);
		codValidacion = validarMensaje(mensaje, KERNEL, logger_KERNEL);
		printf("Codigo de validacion: %d \n", codValidacion);
		if (codValidacion != EXIT_FAILURE) {
			request = separarString(mensaje);
			cod_request = obtenerCodigoPalabraReservada(request[0], KERNEL);
			// El paquete tiene el cod_request y UN request completo
			paquete = armar_paquete(cod_request, mensaje);
			printf("Voy a enviar este cod: %d \n", paquete->palabraReservada);
			log_info(logger_KERNEL, "Antes de enviar mensaje");
			enviar(paquete, conexion);
			free(paquete);
			log_info(logger_KERNEL, "despues de enviar mensaje");
		}
		free(mensaje);
		//config_destroy(config);
		sem_post(&semLeerDeConsola);
	}
	liberar_conexion(conexion);

}
