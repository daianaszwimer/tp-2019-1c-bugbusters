#include "kernel.h"

int main(void) {
	logger_KERNEL = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
	log_info(logger_KERNEL, "----------------INICIO DE KERNEL--------------");
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/kernel/kernel.config");

	//Semáforos
	sem_init(&semRequestNew, 0, 0);
	sem_init(&semMColaNew, 0, 1);

	//Hilos
	pthread_create(&hiloConectarAMemoria, NULL, (void*)conectarAMemoria, NULL);
	pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL);
	pthread_create(&hiloPlanificarNew, NULL, (void*)planificarNew, NULL);
	pthread_create(&hiloPlanificarExec, NULL, (void*)planificarExec, NULL);

	new = queue_create();
	ready = queue_create();
	exec = queue_create();

	pthread_join(hiloLeerDeConsola, NULL);
	pthread_join(hiloConectarAMemoria, NULL);

	liberarMemoria();

	return EXIT_SUCCESS;
}


void leerDeConsola(void){
	char* mensaje;
	while (1) {
		mensaje = readline(">");
		//todo: usar exit
		if (!strcmp(mensaje, "\0")) {
			break;
		}
		sem_wait(&semMColaNew);
		//agregar request a la cola de new
		queue_push(new, mensaje);
		sem_post(&semMColaNew);
		sem_post(&semRequestNew);

		free(mensaje);
	}
}

/*
 Planificación
*/

void planificarNewAReady(void) {
	while(1) {
		sem_wait(&semRequestNew);
		sem_wait(&semMColaNew);
		if(validarRequest(queue_pop(&new))) {
			//queue_pop va con &?
			sem_post(&semMColaNew);
			//validar
			//agregar a
		} else {
			sem_post(&semMColaNew);
			//error
		}
	}
}

void planificarExec(void) {

}

int validarRequest(char* mensaje) {
	int codValidacion = validarMensaje(mensaje, KERNEL, logger_KERNEL);
	switch(codValidacion) {
		case EXIT_SUCCESS:
			return TRUE;
			//manejarRequest(mensaje);
			break;
		case EXIT_FAILURE:
		case QUERY_ERROR:
			return FALSE;
			//informar error
			break;
		default:
			return FALSE;
			break;
	}
}

/*
 * Se ocupa de delegar la request
 * */
void reservarRecursos(char* request) {

}
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
	conexionMemoria = crearConexion(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"));
}

void liberarMemoria(void) {
	liberar_conexion(conexionMemoria);
	config_destroy(config);
	log_destroy(logger_KERNEL);
}

/*
 * Funciones que procesan requests
*/

void enviarMensajeAMemoria(cod_request codRequest, char* mensaje) {
	t_paquete* paquete;
	printf("El mensaje es: %s \n", mensaje);
	// El paquete tiene el cod_request y UN request completo
	paquete = armar_paquete(codRequest, mensaje);
	printf("Voy a enviar este cod: %d \n", paquete->palabraReservada);
	enviar(paquete, conexionMemoria);
	free(paquete);
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
		char request[100];
		while(fgets(request, sizeof(request), archivoLql) != NULL) {
			request[strcspn(request, "\n")] = 0;
			validarRequest(request);
		}
		fclose(archivoLql);
	}
}
