#include "kernel.h"

int main(void) {
	inicializarVariables();

	log_info(logger_KERNEL, "----------------INICIO DE KERNEL--------------");

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

/* inicializarVariables()
 * Parametros:
 * 	-> void
 * Descripcion: inicializa todas las variables que se van a usar.
 * Return:
 * 	-> :: void  */
void inicializarVariables() {
	logger_KERNEL = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
	// Configs
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/kernel/kernel.config");
	int multiprocesamiento = config_get_int_value(config, "MULTIPROCESAMIENTO");
	quantum = config_get_int_value(config, "QUANTUM");
	// Semáforos
	sem_init(&semRequestNew, 0, 0);
	sem_init(&semRequestReady, 0, 0);
	sem_init(&semMultiprocesamiento, 0, multiprocesamiento);
	pthread_mutex_init(&semMColaNew, NULL);
	pthread_mutex_init(&semMColaReady, NULL);
	// Colas de planificacion
	new = queue_create();
	ready = queue_create();
	// Memorias
	memoriasEc = list_create();
	memoriasShc = list_create();
	memorias = list_create();
	memoriaSc = (config_memoria*) malloc(sizeof(config_memoria));
	memoriaSc = NULL;
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
	procesarHandshake(handshake);
}

/* procesarHandshake()
 * Parametros:
 * 	-> t_handshake_memoria* :: handshakeRecibido
 * Descripcion: guarda las memorias levantadas en una lista.
 * Return:
 * 	-> :: void  */
void procesarHandshake(t_handshake_memoria* handshakeRecibido) {
	size_t i = 0;
	char** ips = string_split(handshakeRecibido->ips, ",");
	char** puertos = string_split(handshakeRecibido->puertos, ",");
	//char** numeros = string_split(handshakeRecibido->numeros, ",");
	for( i = 0; ips[i] != NULL; i++)
	{
		config_memoria* memoriaNueva = (config_memoria*) malloc(sizeof(config_memoria));
		memoriaNueva->ip = config_get_string_value(config, "IP_MEMORIA"); //ips[i];
		memoriaNueva->puerto = config_get_string_value(config, "PUERTO_MEMORIA"); //puertos[i];
		memoriaNueva->numero = "1";
		list_add(memorias, memoriaNueva);
	}
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
		/*if (!strcmp(mensaje, "\0")) {
			printf("sali de leer consola");
			free(mensaje);
			mensaje = NULL;
			break;
		}*/
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
		char* request = strdup((char*) queue_pop(new));
//		char* request = (char*) malloc(strlen((char*)queue_peek(new)) + 1);
//		request = (char*) queue_pop(new);
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
	request_procesada* _request = (request_procesada*) malloc(sizeof(request_procesada));
	if (codigo == RUN) {
		//desarmo archivo y lo pongo en una cola
		FILE *archivoLql;
		char** parametros;
		parametros = separarRequest(mensaje);
		archivoLql = fopen(parametros[1], "r");
		if (archivoLql == NULL) {
			log_error(logger_KERNEL, "No existe un archivo en esa ruta");
		} else {
			_request->request = queue_create();
			char letra;
			char* request;
			char** requestDividida;
			int i = 1;
			size_t posicion = 0;
			request = (char*) malloc(sizeof(char));
		    *request = '\0';
			while((letra = fgetc(archivoLql)) != EOF) {
				if (letra != '\n') {
					// concateno
					i++;
					request = (char*) realloc(request, i * sizeof(char));
					posicion = strlen(request);
					request[posicion] = letra;
					request[posicion + 1] = '\0';
				} else {
					request_procesada* otraRequest = (request_procesada*) malloc(sizeof(request_procesada));
					requestDividida = string_n_split(request, 2, " ");
					cod_request _codigo = obtenerCodigoPalabraReservada(requestDividida[0], KERNEL);
					otraRequest->codigo = _codigo;
					otraRequest->request = strdup(request);
					queue_push(_request->request, otraRequest);
					liberarArrayDeChar(requestDividida);
					i = 1;
					request = (char*) malloc(sizeof(char));
				    *request = '\0';
				}
			}
			if (i != 1) { //hack horrible para que funcione cuando los lql tienen salto de linea al final y cuando no tmb
				//esto esta para la ultima linea, porque como no tiene salto de linea no entra al else y no se guarda sino
				request_procesada* otraRequest = (request_procesada*) malloc(sizeof(request_procesada));
				requestDividida = string_n_split(request, 2, " ");
				cod_request _codigo = obtenerCodigoPalabraReservada(requestDividida[0], KERNEL);
				otraRequest->codigo = _codigo;
				otraRequest->request = strdup(request);
				queue_push(_request->request, otraRequest);
				liberarArrayDeChar(requestDividida);
			}
			liberarArrayDeChar(parametros);
			fclose(archivoLql);
		}
		_request->codigo = RUN;
		pthread_mutex_lock(&semMColaReady);
		queue_push(ready, _request);
		pthread_mutex_unlock(&semMColaReady);
	} else {
		_request->request = strdup(mensaje);
		_request->codigo = codigo;
		pthread_mutex_lock(&semMColaReady);
		queue_push(ready, _request);
		pthread_mutex_unlock(&semMColaReady);
	}
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
		request = (request_procesada*) malloc(sizeof(request_procesada));
		pthread_mutex_lock(&semMColaReady);
		request->codigo = ((request_procesada*) queue_peek(ready))->codigo;
		if(request->codigo == RUN) {
			request->request = (t_queue*) malloc(sizeof(((request_procesada*)queue_peek(ready))->request));
			request->request = ((request_procesada*)queue_peek(ready))->request;
		} else {
			request->request = strdup((char*)((request_procesada*)queue_peek(ready))->request);
		}
		queue_pop(ready);
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
	// si no hay mas elementos en la cola, hago free(request)
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
int manejarRequest(request_procesada* request) {
	int respuesta = 0;
	//devolver la respuesta para usarla en el run
	switch(request->codigo) {
		case SELECT:
		case INSERT:
		case CREATE:
		case DESCRIBE:
		case DROP:
			respuesta = enviarMensajeAMemoria(request->codigo, consistenciaMemoria, (char*) request->request);
			break;
		case JOURNAL:
			// solo a memorias que tengan un criterio
			break;
		case ADD:
			respuesta = procesarAdd((char*) request->request);
			break;
		case RUN:
			procesarRun((t_queue*) request->request);
			break;
		case METRICS:
			break;
		default:
			// aca puede entrar solo si viene de run, porque sino antes siempre fue validada
			// la request. en tal caso se devuelve el t_paquete con error para cortar ejecucion
			break;
	}
	usleep(config_get_int_value(config, "SLEEP_EJECUCION")*1000);
	return respuesta;
}

config_memoria* encontrarMemoriaSegunTabla(char* tabla) {
	//busco en mi estructura de tablas el tipo
	consistencia consistenciaDeTabla = SC;
	config_memoria* memoriaCorrespondiente = (config_memoria*) malloc(sizeof(config_memoria));
	switch(consistenciaDeTabla) {
		case SC:
			if (memoriaSc == NULL) {
				log_error(logger_KERNEL, "No se puede resolver el request porque no hay memorias asociadas al criterio SC");
				memoriaCorrespondiente->ip = strdup("-1");
			} else {
				memoriaCorrespondiente = memoriaSc;
			}
			break;
		case SHC:
			break;
		case EC:
			break;
		default:
			//error
			break;
	}
	return memoriaCorrespondiente;
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
	queue_destroy(new);
	queue_destroy(ready);
	list_destroy(memorias);
	list_destroy(memoriasShc);
	list_destroy(memoriasEc);
}

/* liberarRequestProcesada()
 * Parametros:
 * 	-> request_procesada* :: request
 * Descripcion: libera los recursos de la request.
 * Return:
 * 	-> void  */
void liberarRequestProcesada(request_procesada* request) {
	if(request->codigo != RUN) {
       free((char*) request->request);
       request->request = NULL;
	}
	// en el caso del run ya se libera la cola en su funcion
	// solo hacer este free si request fue a exit
	free(request);
	request = NULL;
}

/* liberarColaRequest()
 * Parametros:
 *     -> request_procesada* :: requestCola
 * Descripcion: recibe una request que se encuentra dentro de una cola y la libera.
 * Return:
 *     -> void  */
void liberarColaRequest(request_procesada* requestCola) {
	//printf("Voy a liberar: %s    ", (char*) requestCola->request);
    free((char*) requestCola->request);
    requestCola->request = NULL;
    free(requestCola);
    requestCola = NULL;
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
int enviarMensajeAMemoria(cod_request codigo, consistencia consistenciaMemoria, char* mensaje) {
	//codigo se va a usar para el describe? para cuando es global?
	t_paquete* paqueteRecibido;
	int respuesta;
	int conexionTemporanea;
	char** parametros = separarRequest(mensaje);
	config_memoria* memoriaCorrespondiente = encontrarMemoriaSegunTabla(parametros[1]);
	//buscar a que memoria mandarle el dato, crear conexion, mandarselo y cerrar conexion, sii no es la ppal
	if(string_equals_ignore_case(memoriaCorrespondiente->ip, "-1")) {
		respuesta = ERROR_GENERICO;
		free(memoriaCorrespondiente->ip);
	} else {
		if (string_equals_ignore_case(memoriaCorrespondiente->ip, config_get_string_value(config, "IP_MEMORIA"))
			&& string_equals_ignore_case(memoriaCorrespondiente->puerto, config_get_string_value(config, "PUERTO_MEMORIA"))) {
			conexionTemporanea = conexionMemoria;
		} else {
			conexionTemporanea = crearConexion(memoriaCorrespondiente->ip, memoriaCorrespondiente->puerto);
		}
		enviar(consistenciaMemoria, mensaje, conexionTemporanea);
		paqueteRecibido = recibir(conexionTemporanea);
		respuesta = paqueteRecibido->palabraReservada;
		if (respuesta == SUCCESS) {
			log_info(logger_KERNEL, "La respuesta del request %s es %s \n", mensaje, paqueteRecibido->request);
		} else {
			log_error(logger_KERNEL, "El request %s no es válido", mensaje);
		}
		if (conexionTemporanea != conexionMemoria) {
			liberar_conexion(conexionTemporanea);
		}
		eliminar_paquete(paqueteRecibido);
	}
	//free(memoriaCorrespondiente);
	liberarArrayDeChar(parametros);
	return respuesta;
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
	request_procesada* request;
	int quantumActual = 0;
	//el while es fin de Q o fin de cola
	while(!queue_is_empty(colaRun) && quantumActual < quantum) {
		request = (request_procesada*) malloc(sizeof(request_procesada));
		request->codigo = ((request_procesada*) queue_peek(colaRun))->codigo;
		request->request = strdup((char*)((request_procesada*)queue_peek(colaRun))->request);

		if (validarRequest((char*) request->request) == TRUE) {
			if (manejarRequest(request) != SUCCESS) {
				break;
				//libero recursos, mato hilo, lo saco de la cola, e informo error
			}
		} else {
			break;
			//libero recursos, mato hilo, lo saco de la cola, e informo error
		}
		queue_pop(colaRun);
		liberarRequestProcesada(request);
		quantumActual++;
	}
	if (quantumActual == quantum && queue_is_empty(colaRun) == FALSE) {
		//termino por fin de q
		log_info(logger_KERNEL, "Vuelvo a ready");
		request_procesada* _request = (request_procesada*)(malloc(sizeof(request_procesada)));
		_request->request = (t_queue*) (malloc(sizeof(t_queue)));
		_request->request = colaRun;
		_request->codigo = RUN;
		pthread_mutex_lock(&semMColaReady);
		queue_push(ready, _request);
		pthread_mutex_unlock(&semMColaReady);
		sem_post(&semRequestReady);
	} else if (queue_is_empty(colaRun) == TRUE){
		// si estoy aca es porque ya ejecuto toda la cola
		log_info(logger_KERNEL, "Finalizó la ejecución del script");
		queue_destroy(colaRun);
	} else {
		log_error(logger_KERNEL, "Se cancelo el script");
		liberarRequestProcesada(request);
		queue_destroy_and_destroy_elements(colaRun, (void*)liberarColaRequest);
		// liberar recursos que ocupa el struct este con sus colas o sea el request - es lo de arriba
	}
}

/* procesarAdd()
 * Parametros:
 * 	-> char* :: mensaje
 * Descripcion: recibe el request y asigna criterio a memoria.
 * Luego le informa a memoria que se le asignó ese criterio.
 * Return:
 * 	-> void  */
int procesarAdd(char* mensaje) {
	int estado = SUCCESS;
	char** requestDividida = separarRequest(mensaje);
	config_memoria* memoria = (config_memoria*) malloc(sizeof(config_memoria));
	consistencia _consistencia;
	_consistencia = obtenerEnumConsistencia(requestDividida[4]);
	if (_consistencia == CONSISTENCIA_INVALIDA) {
		estado = ERROR_GENERICO;
		log_error(logger_KERNEL, "El criterio %s no es válido", requestDividida[4]);
	}
	int esMemoriaCorrecta(config_memoria* memoriaActual) {
		return string_equals_ignore_case(memoriaActual->numero, requestDividida[2]);
	}
	memoria = (config_memoria*) list_find(memorias, (void*)esMemoriaCorrecta);
	if (memoria == NULL) {
		estado = ERROR_GENERICO;
		log_error(logger_KERNEL, "No encontré la memoria %s", requestDividida[2]);
	} else {
		switch (_consistencia) {
			case SC:
				memoriaSc = memoria;
				break;
			case SHC:
				list_add(memoriasShc, memoria);
				break;
			case EC:
				list_add(memoriasEc, memoria);
				break;
			default:
				break;
		}
	}
	//free(mensaje);
	liberarArrayDeChar(requestDividida);
	//free(memoria); hay que hacer este free???
	return estado;
}
