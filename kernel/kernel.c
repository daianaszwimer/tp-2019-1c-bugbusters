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

/* conectarAMemoria()
 * Parametros:
 * 	-> void
 * Descripcion: conecta kernel con memoria.
 * Return:
 * 	-> :: void  */
void conectarAMemoria(void) {
	t_handshake_memoria* handshake;
	conexionMemoria = crearConexion(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"));
	handshake = recibirHandshakeMemoria(conexionMemoria);
	procesarAdd("");
}

/* leerDeConsola()
 * Parametros:
 * 	-> void
 * Descripcion: es un hilo que lee input de consola y agrega los requests a cola de new.
 * Return:
 * 	-> :: void  */
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
	}
}

/*
 Planificación
*/

/* planificarNewAReady()
 * Parametros:
 * 	-> void
 * Descripcion: hilo que, luego de que un request es agregado a la cola de new, lo toma
 * y reserva recursos si y solo si es válido - en el caso de RUN solo valida si el RUN es válido,
 * no las requests dentro del archivo.
 * Return:
 * 	-> :: void  */
void planificarNewAReady(void) {
	while(1) {
		sem_wait(&semRequestNew);
		pthread_mutex_lock(&semMColaNew);
		char* request = malloc(strlen((char*)queue_peek(new)) + 1);
		request = queue_pop(new);
		pthread_mutex_unlock(&semMColaNew);
		if(validarRequest(request) == TRUE) {
			//cuando es run, en vez de pushear request, se pushea array de requests, antes se llama a reservar recursos que hace eso
			reservarRecursos(request);
		} else {
			//error
		}
	}
}

/* reservarRecursos()
 * Parametros:
 * 	-> char* :: mensaje
 * Descripcion: toma request y reserva recursos. Si es RUN lee el archivo y va agregando a una cola,
 * despues esta es agregada a la cola de ready,
 * sino, simplemente lo agrega a la cola de ready.
 * Return:
 * 	-> :: void  */
void reservarRecursos(char* mensaje) {
	char** request = string_n_split(mensaje, 2, " ");
	cod_request codigo = obtenerCodigoPalabraReservada(request[0], KERNEL);
	request_procesada* _request = malloc(sizeof(request_procesada));
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
			char letra;
			char* request = (char *) malloc(sizeof(char));
			char** requestDividida = NULL;
			int i = 0;
			while((letra = fgetc(archivoLql)) != EOF) {
				if (letra != '\n') {
					// concateno
					request = (char *) realloc(request, sizeof(letra));
					request[i] = letra;
					i++;
				} else {
					request_procesada* otraRequest = malloc(sizeof(request_procesada));
					request[i] = '\0';
					requestDividida = string_n_split(request, 2, " ");
					cod_request codigo = obtenerCodigoPalabraReservada(requestDividida[0], KERNEL);
					void* requestRecibido = malloc(sizeof(request));
					otraRequest->request = requestRecibido;
					otraRequest->codigo = codigo;
					otraRequest->request = request;
					queue_push(colaRun, otraRequest);
					free(request);
					free(requestDividida[0]);
					free(requestDividida[1]);
					free(requestDividida);
					liberarRequestProcesada(otraRequest);
					request = malloc(sizeof(char));
					i = 0;
				}
			}
			free(request);
			fclose(archivoLql);
		}
		void* requestRecibido = malloc(sizeof(colaRun));
		_request->request = requestRecibido;
		_request->codigo = RUN;
		_request->request = colaRun;
		pthread_mutex_lock(&semMColaReady);
		queue_push(ready, _request);
		pthread_mutex_unlock(&semMColaReady);
	} else {
		void* requestRecibido = malloc(sizeof(mensaje));
		_request->request = requestRecibido;
		_request->codigo = codigo;
		_request->request = mensaje;
		pthread_mutex_lock(&semMColaReady);
		queue_push(ready, _request);
		pthread_mutex_unlock(&semMColaReady);
	}
	liberarRequestProcesada(_request);
	sem_post(&semRequestReady);
}

/* planificarReadyAExec()
 * Parametros:
 * 	-> void
 * Descripcion: hilo que, luego de que un request es agregado a la cola de ready, lo toma
 * y si no se superó el nivel de multiprocesamiento, lo ejecuta y lo agrega a la cola de exec.
 * Return:
 * 	-> :: void  */
void planificarReadyAExec(void) {
	pthread_t hiloRequest;
	pthread_attr_t attr;
	request_procesada* request;
	int threadProcesar;
	while(1) {
		sem_wait(&semRequestReady);
		sem_wait(&semMultiprocesamiento);
		request = malloc(sizeof(request_procesada));
		// esto tira invalid read
		if(((request_procesada*)queue_peek(ready))->codigo == RUN) {
			request->request = malloc(sizeof((request_procesada*)queue_peek(ready))->request);
		} else {
			request->request = malloc(strlen(((request_procesada*)queue_peek(ready))->request) + 1);
		}
		//hiloRequest = malloc(sizeof(pthread_t)); esto va?
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
	}
}

/* procesarRequest()
 * Parametros:
 * 	-> request_procesada* :: request
 * Descripcion: hilo que se crea dinámicamente, desde planificarReadyAExec, maneja la request y libera sus recursos.
 * Return:
 * 	-> :: void  */
void procesarRequest(request_procesada* request) {
	manejarRequest(request);
	//printf("aa  ");
	//fflush(stdout);
	liberarRequestProcesada(request);
	sem_post(&semMultiprocesamiento);
}

/* validarRequest()
 * Parametros:
 * 	-> char* :: mensaje
 * Descripcion: toma un request y se fija si es válido.
 * Return:
 * 	-> requestEsValida :: bool  */
int validarRequest(char* mensaje) {
	//aca se va a poder validar que se haga select/insert sobre tabla existente?
	int codValidacion = validarMensaje(mensaje, KERNEL, logger_KERNEL);
	switch(codValidacion) {
		case EXIT_SUCCESS:
			return TRUE;
			break;
		case EXIT_FAILURE:
		case NUESTRO_ERROR:
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

/* manejarRequest()
 * Parametros:
 * 	-> request_procesada* :: request
 * Descripcion: toma un request y se fija a quién delegarselo, toma la respuesta y la devuelve.
 * Return:
 * 	-> respuesta :: t_paquete*  */
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
			//procesarRun(request->request);
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

/* liberarMemoria()
 * Parametros:
 * 	-> void
 * Descripcion: libera los recursos.
 * Return:
 * 	-> void  */
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

/* liberarRequestProcesada()
 * Parametros:
 * 	-> request_procesada* :: request
 * Descripcion: libera los recursos de la request.
 * Return:
 * 	-> void  */
void liberarRequestProcesada(request_procesada* request) {
	// segun valgrind este free no hace falta
	// solo liberar cuando es un run
	// free(request->request);
	free(request);
}

/*
 * Funciones que procesan requests
*/

/* enviarMensajeAMemoria()
 * Parametros:
 * 	-> cod_request :: codRequest
 * 	-> char* :: mensaje
 * Descripcion: recibe una request, se la manda a memoria y recibe la respuesta.
 * Return:
 * 	-> paqueteRecibido :: t_paquete*  */
t_paquete* enviarMensajeAMemoria(cod_request codRequest, char* mensaje) {
	t_paquete* paqueteRecibido;
	//en enviar habria que enviar puerto para que memoria sepa a cual se le delega el request segun la consistency
	enviar(codRequest, mensaje, conexionMemoria);
	free(mensaje);
	paqueteRecibido = recibir(conexionMemoria);
	log_info(logger_KERNEL, "Recibi de memoria %s \n", paqueteRecibido->request);
	return paqueteRecibido;
}

/* procesarRun()
 * Parametros:
 * 	-> t_queue* :: colaRun
 * Descripcion: recibe una cola de requests, procesa cada una,
 * si es válida la ejecuta. Cuando es fin de quantum, vuelve a ready.
 * Return:
 * 	-> void  */
void procesarRun(t_queue* colaRun) {
	// usar inotify por si cambia Q
	t_paquete* respuesta;
	request_procesada* request;
	int quantumActual = 0;
	//el while va a ser fin de Q o fin de cola
	while(!queue_is_empty(colaRun) || quantumActual < quantum) {
		request = queue_pop(colaRun);
		if (validarRequest(request->request)) {
			respuesta = manejarRequest(request);
			if (respuesta->palabraReservada == NUESTRO_ERROR) {
				//libero recursos, mato hilo, lo saco de la cola, e informo error
			}
			eliminar_paquete(respuesta);
		} else {
			//libero recursos, mato hilo, lo saco de la cola, e informo error
		}
		quantumActual++;
	}
}

/* procesarAdd()
 * Parametros:
 * 	-> char* :: mensaje
 * Descripcion: recibe el request y asigna criterio a memoria.
 * Luego le informa a memoria que se le asignó ese criterio.
 * Return:
 * 	-> void  */
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
