#include "kernel.h"

int main(void) {
	logger_KERNEL = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
	log_info(logger_KERNEL, "----------------INICIO DE KERNEL--------------");

	//	SEMAFOROS
	//sem_init(&semLeerDeConsola, 0, 1);
	//sem_init(&semEnviarMensajeAMemoria, 0, 0);

	//HILOS
	pthread_create(&hiloConectarAMemoria, NULL, (void*)conectarAMemoria, NULL);
	pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL);

	//enviarMensajeAMemoria();

	pthread_join(hiloLeerDeConsola, NULL);
	pthread_join(hiloConectarAMemoria, NULL);

	liberarMemoria();

	return EXIT_SUCCESS;
}


void leerDeConsola(void){
	//log_info(logger, "leerDeConsola");
	char* mensaje;
	while (1) {
		//sem_wait(&semLeerDeConsola);
		mensaje = readline(">");
		//todo: usar exit
		if (!strcmp(mensaje, "\0")) {
			//sem_post(&semEnviarMensajeAMemoria);
			break;
		}
		interpretarRequest(mensaje);
	}
}

/*
 * Se ocupa de validar y si esta todo bien, delega
 * */
void interpretarRequest(char* mensaje) {
	int codValidacion = validarMensaje(mensaje, KERNEL, logger_KERNEL);
	switch(codValidacion) {
		case EXIT_SUCCESS:
			manejarRequest(mensaje);
			break;
		case EXIT_FAILURE:
		case QUERY_ERROR:
			//informar error
			break;
		default:
			break;
	}
}

/*
 * Se ocupa de delegar la request
 * */
void manejarRequest(char* mensaje) {
	cod_request codRequest; // es la palabra reservada (ej: SELECT)
	char** request;
	request = separarString(mensaje);
	codRequest = obtenerCodigoPalabraReservada(request[0], KERNEL);
	switch(codRequest) {
		case SELECT:
		case INSERT:
		case CREATE:
		case DESCRIBE:
		case DROP:
		case JOURNAL:
			enviarMensajeAMemoria(codRequest, mensaje);
			break;
		case ADD:
			break;
		case RUN:
			procesarRun(mensaje);
			break;
		case METRICS:
			break;
		default:
			break;
	}
}

void conectarAMemoria(void) {
	//todo: esperar que se levante memoria
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/kernel/kernel.config");
	//semaforos
	conexionMemoria = crearConexion(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"));
}

void liberarMemoria(void) {
	liberar_conexion(conexionMemoria);
	config_destroy(config);
	log_destroy(logger_KERNEL);
}

/*
 * Funciones que procesan requests
 * */

void enviarMensajeAMemoria(cod_request codRequest, char* mensaje) {
	t_paquete* paquete;
	printf("El mensaje es: %s \n", mensaje);
	// El paquete tiene el cod_request y UN request completo
	paquete = armar_paquete(codRequest, mensaje);
	printf("Voy a enviar este cod: %d \n", paquete->palabraReservada);
	enviar(paquete, conexionMemoria);
	free(paquete);
	free(mensaje);
}

void procesarRun(char* mensaje) {
	FILE *archivoLql;
	char** parametros;
	parametros = obtenerParametros(mensaje);
	printf("ingresaste como archivo %s \n", parametros[0]);
	archivoLql = fopen(parametros[0], "r");
	if (archivoLql == NULL) {
		log_error(logger_KERNEL, "No existe un archivo en esa ruta");
	} else {
		char* palabra;
		//no anda este while
		while(fscanf(archivoLql, "%s", palabra) == 1) {
			//	interpretarRequest(request);
		}
		fclose(archivoLql);
	}
}
