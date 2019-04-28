#include "kernel.h"

int main(void) {
	inicializarVariables();
	log_info(logger, "----------------INICIO DE KERNEL--------------");
	sem_init(&semLeerDeConsola, 0, 1);
	sem_init(&semEnviarMensaje, 0, 0);

	pthread_create(&hiloLeerConsola, NULL, (void*)leerDeConsola, NULL);

	enviarMensajeAMemoria();

	pthread_join(hiloLeerConsola, NULL);
	return EXIT_SUCCESS;
}

void inicializarVariables(){
	logger = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
	log_info(logger, "InicializarVariables");
	t_config* config = leer_config("kernel.config");
	//int conexion = crearConexion(config_get_string_value(config, "IP"), config_get_string_value(config, "PUERTO"));
	conexion = crearConexion("127.0.0.1", "4444");
}


void leerDeConsola(){
	log_info(logger, "LeerDeConsola");
	while (1) {
		sem_wait(&semLeerDeConsola);
		mensaje = readline(">");
		if (!strcmp(mensaje, "\0")) {
			sem_post(&semEnviarMensaje);
			break;
		}
		sem_post(&semEnviarMensaje);
	}
}

void enviarMensajeAMemoria() {
	int cod_request; // es la palabra reservada (ej: SELECT)
	char** request;
	int codValidacion;
	log_info(logger, "enviarMensajeAMemoria");
	while (1) {
		sem_wait(&semEnviarMensaje);
		if (!strcmp(mensaje, "\0")) {
			break;
		}
		printf("El mensaje es: %s \n", mensaje);
		codValidacion = validarMensaje(mensaje, KERNEL);
		printf("Codigo de validacion: %d \n", codValidacion);
		if (codValidacion != EXIT_FAILURE) {
			request = separarString(mensaje);
			cod_request = obtenerCodigoPalabraReservada(request[0], KERNEL);
			// El paquete tiene el cod_request y UN request completo
			t_paquete* paquete = armar_paquete(cod_request, mensaje);
			printf("Voy a enviar este cod: %d \n", paquete->palabraReservada);
			log_info(logger, "Antes de enviar mensaje");
			enviar(paquete, conexion);
			log_info(logger, "despues de enviar mensaje");
		}

		//log_destroy(logger);
		free(mensaje);
		//config_destroy(config);
		sem_post(&semLeerDeConsola);
	}
	close(conexion);

}
