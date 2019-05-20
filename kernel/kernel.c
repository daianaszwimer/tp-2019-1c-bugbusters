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

	// inicializo variables -> todo: mover a una funcion
	new = queue_create();
	ready = queue_create();
	exec = queue_create();
	memoriasEc = list_create();
	memoriasShc = list_create();
	memorias = list_create();

	//Hilos
	pthread_create(&hiloConectarAMemoria, NULL, (void*)conectarAMemoria, NULL);
	pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL);
	pthread_create(&hiloPlanificarNew, NULL, (void*)planificarNewAReady, NULL);
	pthread_create(&hiloPlanificarExec, NULL, (void*)planificarReadyAExec, NULL);

	pthread_join(hiloConectarAMemoria, NULL);
	pthread_join(hiloLeerDeConsola, NULL);
	pthread_join(hiloPlanificarNew, NULL);
	pthread_join(hiloPlanificarExec, NULL);

	liberarMemoria();

	return EXIT_SUCCESS;
}

void conectarAMemoria(void) {
	conexionMemoria = crearConexion(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"));
	procesarAdd("");
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

void reservarRecursos(char* mensaje) {
	char** request = string_n_split(mensaje, 2, " ");
	cod_request codigo = obtenerCodigoPalabraReservada(request[0], KERNEL);
	request_procesada* _request;
	if (codigo == RUN) {
		//desarmo archivo y lo pongo en una cola
		t_queue* colaRun = queue_create();
		FILE *archivoLql;
		char** parametros;
		parametros = obtenerParametros(mensaje);
		archivoLql = fopen(parametros[0], "r");
		if (archivoLql == NULL) {
			log_error(logger_KERNEL, "No existe un archivo en esa ruta");
		} else {
			// usar memoria dinamica o leer caracter por caracter
			char* request;
			char** requestDividida;
			while(fgets(request, sizeof(request), archivoLql) != NULL) {
				request[strcspn(request, "\n")] = 0;
				requestDividida = string_n_split(request, 2, " ");
				cod_request codigo = obtenerCodigoPalabraReservada(requestDividida[0], KERNEL);
				_request->codigo = codigo;
				_request->request = request;
				queue_push(colaRun, _request);
			}
			fclose(archivoLql);
		}
		_request->codigo = RUN;
		_request->request = colaRun;
		pthread_mutex_lock(&semMColaReady);
		queue_push(ready, _request);
		pthread_mutex_unlock(&semMColaReady);
	} else {
		_request->codigo = codigo;
		_request->request = mensaje;
		pthread_mutex_lock(&semMColaReady);
		queue_push(ready, _request);
		pthread_mutex_unlock(&semMColaReady);
	}
	sem_post(&semRequestReady);
}

void planificarReadyAExec(void) {
	pthread_t hiloRequest;
	pthread_attr_t attr;
	request_procesada* request;
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

void procesarRequest(request_procesada* request) {
	manejarRequest(request);
	printf("aa  ");
	sem_post(&semMultiprocesamiento);
	//free(request);
}

int validarRequest(char* mensaje) {
	//aca se va a poder validar que se haga select/insert sobre tabla existente?
	int codValidacion = validarMensaje(mensaje, KERNEL, logger_KERNEL);
	switch(codValidacion) {
		case EXIT_SUCCESS:
			return TRUE;
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
*/

t_paquete* manejarRequest(request_procesada* request) {
	t_paquete* respuesta;
	//devolver la respuesta para usarla en el run
	switch(request->codigo) {
		case SELECT:
		case INSERT:
		case CREATE:
		case DESCRIBE:
		case DROP:
		case JOURNAL:
			respuesta = enviarMensajeAMemoria(request->codigo, request->request);
			break;
		case ADD:
			procesarAdd(request->request);
			break;
		case RUN:
			procesarRun(request->request);
			break;
		case METRICS:
			break;
		default:
			// aca puede entrar solo si viene de run, porque sino antes siempre fue validada
			// la request. en tal caso se devuelve el t_paquete con error para cortar ejecucion
			break;
	}
	return respuesta;
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
	//en enviar habria que enviar puerto para que memoria sepa a cual se le delega el request segun la consistency
	enviar(codRequest, mensaje, conexionMemoria);
	free(mensaje);
	paqueteRecibido = recibir(conexionMemoria);
	log_info(logger_KERNEL, "Recibi de memoria %s \n", paqueteRecibido->request);
	return paqueteRecibido;
}

void procesarRun(t_queue* colaRun) {
	// usar inotify por si cambia Q
	t_paquete* respuesta;
	request_procesada* request;
	int quantumActual;
	//el while va a ser fin de Q o fin de cola
	while(!queue_is_empty(colaRun) || quantumActual < quantum) {
		request = queue_pop(colaRun);
		if (validarRequest(request->request)) {
			respuesta = manejarRequest(request);
			if (respuesta->palabraReservada == QUERY_ERROR) {
				//libero recursos, mato hilo, lo saco de la cola, e informo error
			}
		} else {
			//libero recursos, mato hilo, lo saco de la cola, e informo error
		}
		quantumActual++;
	}
}

void procesarAdd(char* mensaje) {
	memoriaSc.ip = config_get_string_value(config, "IP_MEMORIA");
	memoriaSc.puerto = config_get_string_value(config, "PUERTO_MEMORIA");
	/*
	char** parametros = obtenerParametros(mensaje);
	if (list_size(memorias) < (int)parametros[1]) {
		// error
	}
	config_memoria memoria;
	consistencia _consistencia;
	// la memoria n que se manda en el request es la n en la lista
	memoria = list_get(memorias, (int)parametros[1] - 1);
	_consistencia = obtenerEnumConsistencia(parametros[3]);
	if (_consistencia == CONSISTENCIA_INVALIDA) {
		// error
	}
	switch (_consistencia) {
		case SC:
			memoriaSc.ip = memoria.ip;
			memoriaSc.puerto = memoria.puerto;
			break;
		case SHC:
			list_add(memoriasShc, memoria);
			break;
		case EC:
			list_add(memoriasEc, memoria);
			break;
	}
	free(mensaje);
	*/
	//validar que int sea menor al tamaño de mi lista -DONE
	//en la lista se van a encontrar las memorias levantadas - HAY QUE HACER DESCRIBE GLOBAL Y AGREGARLAS
	//y va a ser una lista de config_memoria
	//hacer switch con cada case un criterio - DONE
	//crear tipo de dato criterio que tenga los 3 criterios - DONE
	//para esta iteracion vamos a llamar al add en el conectar
	//y guardar en SC la memoria que se encuentra en el config
}
