#include "kernel.h"

int main(void) {
	logger_KERNEL = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
	log_info(logger_KERNEL, "----------------INICIO DE KERNEL--------------");
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/kernel/kernel.config");
	int multiprocesamiento = config_get_int_value(config, "MULTIPROCESAMIENTO");
	//Semáforos
	sem_init(&semRequestNew, 0, 0);
	sem_init(&semRequestReady, 0, 0);
	sem_init(&semMultiprocesamiento, 0, multiprocesamiento);
	pthread_mutex_init(&semMColaNew, NULL);
	pthread_mutex_init(&semMColaReady, NULL);

	new = queue_create();
	ready = queue_create();
	exec = queue_create();

	//Hilos
	pthread_create(&hiloConectarAMemoria, NULL, (void*)conectarAMemoria, NULL);
	pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL);
	pthread_create(&hiloPlanificarNew, NULL, (void*)planificarNewAReady, NULL);
	pthread_create(&hiloPlanificarExec, NULL, (void*)planificarReadyAExec, NULL);

	pthread_join(hiloConectarAMemoria, NULL);
	pthread_join(hiloPlanificarNew, NULL);
	pthread_join(hiloPlanificarExec, NULL);
	pthread_join(hiloLeerDeConsola, NULL);

	liberarMemoria();

	return EXIT_SUCCESS;
}

void conectarAMemoria(void) {
	conexionMemoria = crearConexion(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"));
	procesarAdd(0);
}

void leerDeConsola(void){
	char* mensaje;
	while (1) {
		mensaje = readline(">");
		//todo: usar exit
		if (!strcmp(mensaje, "\0")) {
			printf("sali de leer consola");
			break;
		}
		pthread_mutex_lock(&semMColaNew);
		//agregar request a la cola de new
		queue_push(new, mensaje);
		pthread_mutex_unlock(&semMColaNew);
		sem_post(&semRequestNew);
		//si descomento esto, tira segmentation fault -> free(mensaje);
	}
}

/*
 Planificación
*/

void planificarNewAReady(void) {
	while(1) {
		sem_wait(&semRequestNew);
		pthread_mutex_lock(&semMColaNew);
		char* request = queue_pop(new);
		pthread_mutex_unlock(&semMColaNew);
		if(validarRequest(request) == TRUE) {
			//cuando es run, en vez de pushear request, se pushea array de requests, antes se llama a reservar recursos que hace eso
			reservarRecursos(request);
		} else {
			//error
		}
	}
}

void reservarRecursos(char* request) {
	pthread_mutex_lock(&semMColaReady);
	queue_push(ready, request);
	pthread_mutex_unlock(&semMColaReady);
	sem_post(&semRequestReady);
	//desarmar archivo lql y ponerlo en una cola
}

void planificarReadyAExec(void) {
	pthread_t hiloRequest;
	pthread_attr_t attr;
	char* request;
	int threadProcesar;
	while(1) {
		sem_wait(&semRequestReady);
		sem_wait(&semMultiprocesamiento);
		//hiloRequest = malloc(sizeof(pthread_t));
		pthread_mutex_lock(&semMColaReady);
		request = queue_pop(ready);
		pthread_mutex_unlock(&semMColaReady);
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		threadProcesar = pthread_create(&hiloRequest, &attr, (void*)procesarRequest, request);
		if(threadProcesar == 0){
			pthread_attr_destroy(&attr);
		} else {
			//informar error
		}
		//if es individual -> un hilo
		//si es run -> otro hilo
		//se crea hilo para el request y dentro del hilo se hace
		//el post que esta abajo
	}
	//cuando se ejecuta una sola linea se hace lo mismo que cuando se ejcuta una linea de un archivo de RUN
}

void procesarRequest(char* request) {
	printf("aa  ");
	sem_post(&semMultiprocesamiento);
	free(request);
}

int validarRequest(char* mensaje) {
	//aca se va a poder validar que se haga select/insert sobre tabla existente?
	int codValidacion = validarMensaje(mensaje, KERNEL, logger_KERNEL);
	switch(codValidacion) {
		case EXIT_SUCCESS:
			return TRUE;
			break;
		case EXIT_FAILURE:
		case NUESTRO_ERROR:
			//informar error
			return FALSE;
			break;
		default:
			return FALSE;
			break;
	}
}

/*
 * Se ocupa de delegar la request
*/

void manejarRequest(char* mensaje) {
	cod_request codRequest; // es la palabra reservada (ej: SELECT)
	char** request;
	t_paquete* respuesta;
	request = separarString(mensaje);
	codRequest = obtenerCodigoPalabraReservada(request[0], KERNEL);
	switch(codRequest) {
		case SELECT:
		case INSERT:
		case CREATE:
		case DESCRIBE:
		case DROP:
		case JOURNAL:
			respuesta = enviarMensajeAMemoria(codRequest, mensaje);
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

void liberarMemoria(void) {
	liberar_conexion(conexionMemoria);
	config_destroy(config);
	log_destroy(logger_KERNEL);
	pthread_mutex_destroy(&semMColaNew);
	pthread_mutex_destroy(&semMColaReady);
	sem_destroy(&semRequestNew);
	sem_destroy(&semRequestReady);
	sem_destroy(&semMultiprocesamiento);
}

/*
 * Funciones que procesan requests
*/

t_paquete* enviarMensajeAMemoria(cod_request codRequest, char* mensaje) {
	t_paquete* paqueteRecibido;
	//en enviar habria que enviar puerto para que memoria sepa a quien le delega el request segun la consistency
	enviar(codRequest, mensaje, conexionMemoria);
	free(mensaje);
	paqueteRecibido = recibir(conexionMemoria);
	log_info(logger_KERNEL, "Recibi de memoria %s \n", paqueteRecibido->request);
	return paqueteRecibido;
}

void procesarRun(char* mensaje) {
	FILE *archivoLql;
	char** parametros;
	cod_request codRequest; // es la palabra reservada (ej: SELECT)
	char** requestSeparada;
	t_paquete* respuesta;
	parametros = obtenerParametros(mensaje);
	printf("ingresaste como archivo %s \n", parametros[0]);
	archivoLql = fopen(parametros[0], "r");
	if (archivoLql == NULL) {
		log_error(logger_KERNEL, "No existe un archivo en esa ruta");
	} else {
		char request[100];
		while(fgets(request, sizeof(request), archivoLql) != NULL) {
			request[strcspn(request, "\n")] = 0;
			if (validarRequest(request)) {
				requestSeparada = separarString(mensaje);
				codRequest = obtenerCodigoPalabraReservada(requestSeparada[0], KERNEL);
				respuesta = enviarMensajeAMemoria(codRequest, mensaje);
				if (respuesta->palabraReservada == NUESTRO_ERROR) {
					//libero recursos, mato hilo, lo saco de la cola, e informo error
				}
			} else {
				//libero recursos, mato hilo, lo saco de la cola, e informo error
			}
		}
		fclose(archivoLql);
	}
}

void procesarAdd(int memoria) {
	//validar que int sea menor al tamaño de mi lista
	//en la lista se van a encontrar las memorias levantadas
	//y va a ser una lista de config_memoria
	//hacer switch con cada case un criterio
	//crear tipo de dato criterio que tenga los 3 criterios
	//para esta iteracion vamos a llamar al add en el conectar
	//y guardar en SC la memoria que se encuentra en el config
	memoriaSc.ip = config_get_string_value(config, "IP_MEMORIA");
	memoriaSc.puerto = config_get_string_value(config, "PUERTO_MEMORIA");
}
